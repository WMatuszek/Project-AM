#include "bmp180.h"

// ------------------------------------------------------------------------------- //

// BMP180 I2C slave address
static const uint8_t ADR_WRITE = 0xEE;
static const uint8_t ADR_READ = 0xEF;

// Register address
static const uint8_t REG_OUT_MSB = 0xF6;
//static const uint8_t REG_OUT_LSB = 0xF7;
//static const uint8_t REG_OUT_XLAB = 0xF8;
static const uint8_t REG_MEASURE = 0xF4;
//static const uint8_t REG_RESET = 0xE0;
static const uint8_t REG_ID = 0xD0;
static const uint8_t REG_CALIB_0 = 0xAA;

// Control register values (measurement initialize)
static const uint8_t REGVAL_M_TEMP = 0x2E;
static const uint8_t REGVAL_M_PRESS_OSS_0	= 0x34;
//static const REGVAL_M_PRESS_OSS_1	= 0x74;
//static const REGVAL_M_PRESS_OSS_2	= 0xB4;
//static const REGVAL_M_PRESS_OSS_3	= 0xF4;

// ------------------------------------------------------------------------------- //

static uint8_t _driverState = 0; 

/*
 * I2C bus interface
 */
static BMP180_I2C_interface i2c;

// Calibration values
static BMP180_calibrationValues calib;
// Currently requested measurement
static BMP180_measure_t currentMeasurement = BMP180_M_NONE;
// Currently set oversampling (?)
static uint16_t currentOSS = 0;

// ------------------------------------------------------------------------------- //

static uint8_t resolveMeasurement(BMP180_measure_t mt){
	if (mt == BMP180_M_NONE) return 0;
	if (mt == BMP180_M_TEMP) return REGVAL_M_TEMP;
	if (mt == BMP180_M_PRESS_OSS_0)	return REGVAL_M_PRESS_OSS_0;
}

// ------------------------------------------------------------------------------- //

/*
 * BMP180_isInitialized
 */
uint8_t BMP180_isReady(void){
	return _driverState;
}

/*
 * BMP180_initialize
 */
uint8_t BMP180_initialize(){
	if (i2c.readByte == 0 || 
		i2c.readBytes == 0 || 
		i2c.writeByte == 0 || 
		i2c.writeBytes == 0){
			return BMP180_RCODE_FAIL; // Uninitialized i2c interface struct
	}
	
	calib.B5 = 2400; // Used for pressure calc, updated every temp calculation
	
	_driverState = 1;

	return BMP180_RCODE_OK;
}

/*
 * BMP180_initialize
 */
uint8_t BMP180_initialize2(BMP180_I2C_interface i2c_interface){
	if (i2c_interface.readByte == 0 || 
		i2c_interface.readBytes == 0 || 
		i2c_interface.writeByte == 0 || 
		i2c_interface.writeBytes == 0){
			return BMP180_RCODE_FAIL; // Passed uninitialized i2c interface struct
	}
		
	calib.B5 = 2400;// Used for pressure calc, updated every temp calculation
	
	_driverState = 1;
	
	return BMP180_RCODE_OK;
}

/*
 * BMP180_getI2CInterfacePtr
 */
BMP180_I2C_interface* BMP180_getI2CInterfacePtr(void){
	return &i2c;
}

/*
 * BMP180_readID
 */
uint8_t BMP180_readID(){
	return i2c.readByte(ADR_WRITE, REG_ID);
}

/*
 * BMP180_readCalibrationData
 */
void BMP180_readCalibrationData(){
	const uint8_t calibDataCnt = 22;
	uint8_t rawCalibData[calibDataCnt];

	i2c.readBytes(ADR_READ, REG_CALIB_0, rawCalibData, calibDataCnt);
	
	// Raw calib to useful data
	calib.AC1 = (int16_t)	((uint16_t)rawCalibData[0]  << 8 | rawCalibData[1]);
	calib.AC2 = (int16_t)	((uint16_t)rawCalibData[2]  << 8 | rawCalibData[3]);
	calib.AC3 = (int16_t)	((uint16_t)rawCalibData[4]  << 8 | rawCalibData[5]);
	calib.AC4 = (uint16_t)	((uint16_t)rawCalibData[6]  << 8 | rawCalibData[7]);
	calib.AC5 = (uint16_t)	((uint16_t)rawCalibData[8]  << 8 | rawCalibData[9]);
	calib.AC6 = (uint16_t)	((uint16_t)rawCalibData[10] << 8 | rawCalibData[11]);
	calib.B1  = (int16_t)	((uint16_t)rawCalibData[12] << 8 | rawCalibData[13]);
	calib.B2  = (int16_t)	((uint16_t)rawCalibData[14] << 8 | rawCalibData[15]);
	calib.MB  = (int16_t)	((uint16_t)rawCalibData[16] << 8 | rawCalibData[17]);
	calib.MC  = (int16_t)	((uint16_t)rawCalibData[18] << 8 | rawCalibData[19]);
	calib.MD  = (int16_t)	((uint16_t)rawCalibData[20] << 8 | rawCalibData[21]);
	
	_driverState = 2;
}

/*
 * BMP180_startMeasurement
 */
uint8_t BMP180_startMeasurement(BMP180_measure_t mt){	
	uint8_t mregval;
	if (mt == BMP180_M_NONE) return BMP180_RCODE_FAIL; // Requested no measurement
	
	mregval = resolveMeasurement(mt); // Resolve config register write value
	
	i2c.writeByte(ADR_WRITE, REG_MEASURE, mregval);
	
	currentMeasurement = mt;

	return BMP180_RCODE_OK;
}

/*
 * BMP180_readResult
 */
int32_t BMP180_readResult(){
	uint8_t rawDataBuffer[3];
	
	if (currentMeasurement == BMP180_M_NONE) 
		return BMP180_NO_NEW_DATA; // Read with no active measurement
	
	i2c.readBytes(ADR_READ, REG_OUT_MSB, rawDataBuffer, 2);
	
	return BMP180_convertMeasurement(currentMeasurement, rawDataBuffer);
}

/*
 * BMP180_convertMeasurement
 */
int32_t BMP180_convertMeasurement(BMP180_measure_t mt, uint8_t *data) {
	if (mt == BMP180_M_TEMP) {
		return calculateTemperature(data);
	}
	if (mt == BMP180_M_PRESS_OSS_0) {
		return calculatePressure(data);
	}
	return BMP180_NO_NEW_DATA; // Convert with no active measurement
}

// ------------------------------------------------------------------------------- //


int32_t calculateTemperature(uint8_t *data){
	int32_t raw = (((uint16_t)data[0])<<8) | data[1];
	int32_t X1 = ((raw - calib.AC6)*calib.AC5)/32768;
	int32_t X2 = ((int32_t)(calib.MC)*2048)/(X1 + calib.MD);
	calib.B5 = X1 + X2;
	return (calib.B5+8)/16;
}


int32_t calculatePressure(uint8_t *data){
	int32_t raw = (((uint16_t)data[0])<<8) | data[1];
	int32_t B6, X1, X2, X3, B3, p;
	uint32_t B4, B7;
	B6 = calib.B5 - 4000;
	X1 = (calib.B2 * ((B6 * B6)/4096))/2048;
	X2 = (calib.AC2 * B6)/2048;
	X3 = X1 + X2;
	B3 = (((calib.AC1*4 + X3)<<currentOSS)+2)/4;
	X1 = (calib.AC3 * B6) / 8192;
	X2 = (calib.B1 * ((B6*B6)/4096))/65536;
	X3 = ((X1+X2)+2)/4;
	B4 = calib.AC4 * (uint32_t)(X3+32768)/32768;
	B7 = ((uint32_t)raw - B3)*(50000>>currentOSS);
	if (B7 < 0x80000000) p = (B7*2)/B4;
	else p = (B7/B4*2);
	X1 = (p/256)*(p/256);
	X1 = (X1 * 3038)/65536;
	X2 = (-7357 * p)/65536;
	p = p + (X1+X2+3791)/16;
	return p;
}























