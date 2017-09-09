#ifndef bmp180_h
#define bmp180_h

#include "stm32f4xx.h"

#define BMP180_NO_NEW_DATA INT32_MAX

#define BMP180_RCODE_OK 	0x01;
#define BMP180_RCODE_FAIL  0x00;

typedef enum {
	BMP180_M_NONE = 0,
	BMP180_M_TEMP = 1,
	BMP180_M_PRESS_OSS_0 = 2
} BMP180_measure_t;

typedef struct BMP180_I2C_interface {
	uint8_t (*readByte)(uint8_t addr, uint8_t reg);
	void (*readBytes)(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t count);
	void (*writeByte)(uint8_t addr, uint8_t reg, uint8_t data);
	void (*writeBytes)(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t count);
} BMP180_I2C_interface;

// BMP180 calibration data - 11 values
typedef struct BMP180_calibrationValues {
	int16_t AC1;
	int16_t AC2;
	int16_t AC3;
	uint16_t AC4;
	uint16_t AC5;
	uint16_t AC6;
	int16_t B1;
	int16_t B2;
	int16_t MB;
	int16_t MC;
	int16_t MD;
	int32_t B5;   // Used for pressure calc, updated every temp calculation
} BMP180_calibrationValues;

/**
	@brief Check for driver initialization
	@retval 2 if ready, 1 if initialized but no calib data, 0 if not
*/
uint8_t BMP180_isInitialized(void);

/**
	@brief Initialize driver - check i2c interface validity
	@note Requires initialized i2c interface stuct
	@retval Retcode BMP180_RCODE_OK or BMP180_RCODE_FAIL
*/
uint8_t BMP180_initialize(void);


/**
	@brief Initialize driver - check i2c interface validity
	@param Initialized i2c interface structure to be used by the driver
	@retval Retcode BMP180_RCODE_OK or BMP180_RCODE_FAIL
*/
uint8_t BMP180_initialize2(BMP180_I2C_interface interface);


/**
	@brief Get pointer to internal BMP180 driver i2c bus interface structure
	@retval Pointer to i2c interface struct allowing for init with i2c library
*/
BMP180_I2C_interface* BMP180_getI2CInterfacePtr(void);


/**
	@brief Read BMP180 calibration data, store in data struct
	@retval Void
*/
void BMP180_readCalibrationData(void);


/**
	@brief Read BMP180 ID register
	@retval BMP180 id
*/
uint8_t BMP180_readID(void);


/**
	@brief Initialize BMP180 measurement of specified type
	@param Measurement type
	@retval Retcode BMP180_RCODE_OK or BMP180_RCODE_FAIL
*/
uint8_t BMP180_startMeasurement(BMP180_measure_t measurementType);


/**
	@brief Read BMP180 measurement result and convert received data
	@retval Conversion result or BMP180_NO_NEW_DATA
	@note temperature: [0.1C] / pressure: [Pa]
*/
int32_t BMP180_readResult(void);


/**	
	@brief Calculate temperature/pressure, determined by currently active measurement
	@reval Conversion result or BMP180_NO_NEW_DATA
	@note temperature: [0.1C] / pressure: [Pa]
*/
int32_t BMP180_convertMeasurement(BMP180_measure_t measurementType, uint8_t *data);


/**
	@note --- Private ---
	
	@brief Calculates temperature [0.1C] using calibration values
*/
int32_t calculateTemperature(uint8_t *data);


/**
	@note --- Private ---
	
	@brief Calculates pressure [Pa] using calibration values
*/
int32_t calculatePressure(uint8_t *data);

#endif
