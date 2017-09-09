#include "tm_stm32f4_emwin.h"
#include "tm_stm32f4_usart.h"
#include "tm_stm32f4_disco.h"
#include "tm_stm32f4_rtc.h"
#include "GUI_screens.h"
#include "button.h"
#include "DIALOG.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "tea5767.h"
#include "bmp180.h"

typedef enum {
	RADIO_KEY = GUI_ID_BUTTON1,
	CLOCK_KEY = GUI_ID_BUTTON2,
	ALARM_KEY = GUI_ID_BUTTON3,
	SETTING_KEY = GUI_ID_BUTTON4
} GUI_key_names;

typedef enum {
	BMP180_NO_FLAG,
	BMP180_INIT,
	BMP180_MEASURE_TEMPERATURE,
	BMP180_MEASURE_PRESSURE,
	BMP180_READ_RESULT_TEMPERATURE,
	BMP180_READ_RESULT_PRESSURE
} BMP180_Flag_t;

typedef enum {
	TEA5767_NO_FLAG,
	TEA5767_SET_FREQUENCY,
	TEA5767_SCAN
} TEA5767_Flag_t;

typedef enum {
	BUS_FREE = 0,
	BUS_BUSY = 1 
} SERIAL_STATE_t;

uint8_t TEA5767_Flags = TEA5767_NO_FLAG;
uint8_t BMP180_Flags = BMP180_NO_FLAG;

// Button handles
BUTTON_Handle hButtons[20];

TM_RTC_Time_t Time;
TM_RTC_AlarmTime_t AlarmTime;

static I2C_TypeDef* _I2Cx = I2C3;

uint16_t _counter = 0x00;

// I2C access flag
SERIAL_STATE_t _I2Cx_state = BUS_FREE;

// Data from sensors
float _freq = 87.8;
int32_t _temp = 0;
int32_t _press = 0;

/*
	BMP180 interfacing functions
*/
uint8_t bmp180compliant_readByte(uint8_t addr, uint8_t reg){
	return TM_I2C_Read(_I2Cx, addr, reg);
}
void bmp180compliant_readBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t count){
	TM_I2C_ReadMulti(_I2Cx, addr, reg, data, count);
}
void bmp180compliant_writeByte(uint8_t addr, uint8_t reg, uint8_t data){
	TM_I2C_Write(_I2Cx, addr, reg, data);
}
void bmp180compliant_writeBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t count){
	TM_I2C_WriteMulti(_I2Cx, addr, reg, data, count);
}

/*
	TEA5767 interfacing functions
*/
void tea5767compliant_writeBytesNoRegister(uint8_t addr, uint8_t *data, uint16_t count){
	TM_I2C_WriteMultiNoRegister(_I2Cx, addr, data, count);
}

void tea5767compliant_readBytesNoRegister(uint8_t addr, uint8_t *data, uint16_t count){
	TM_I2C_ReadMultiNoRegister(_I2Cx, addr, data, count);
}

uint8_t tea5767compliant_isConnected(uint8_t addr){
	return TM_I2C_IsDeviceConnected(_I2Cx, addr);
}

/*
	Fonts/buttons
*/
void deleteButtons(uint8_t start, uint8_t stop){
	uint8_t i;
	for (i = start; i <= stop; i++) BUTTON_Delete(hButtons[i]);
}

void setFonts(uint8_t start, uint8_t stop, const GUI_FONT GUI_UNI_PTR * pfont){
	uint8_t i;
	for (i = start; i <= stop; i++) BUTTON_SetFont(hButtons[i], pfont);
}

/*
	Sensor/radio install interface
*/
GUI_InitResult GUI_TEA5767_Init(I2C_TypeDef* I2Cx){
	TEA5767_I2C_interface* tea5767i2cInterfacePtr;
	
	_I2Cx = I2Cx;
	
	tea5767i2cInterfacePtr = TEA5767_getI2CInterfacePtr();
	tea5767i2cInterfacePtr->writeBytesNoRegister = tea5767compliant_writeBytesNoRegister;
	tea5767i2cInterfacePtr->readBytesNoRegister = tea5767compliant_readBytesNoRegister;
	tea5767i2cInterfacePtr->isConnected = tea5767compliant_isConnected;
		
	TEA5767_Init(); // SCL = PA8, SDA = PC9
	TEA5767_Flags = 0x01;
	return GUI_INIT_OK;
}

GUI_InitResult GUI_BMP180_Init(I2C_TypeDef* I2Cx){
	//char buf[50];
	uint8_t deviceID = 0x00;
	BMP180_I2C_interface * bmp180i2cInterfacePtr;

	_I2Cx = I2Cx;
	
	bmp180i2cInterfacePtr = BMP180_getI2CInterfacePtr();
	bmp180i2cInterfacePtr->readByte = bmp180compliant_readByte;
	bmp180i2cInterfacePtr->readBytes = bmp180compliant_readBytes;
	bmp180i2cInterfacePtr->writeByte = bmp180compliant_writeByte;
	bmp180i2cInterfacePtr->writeBytes = bmp180compliant_writeBytes;
	
	//BMP180_Flags = BMP180_INIT;
	_I2Cx_state = BUS_BUSY;
	BMP180_initialize2(*bmp180i2cInterfacePtr);
	_I2Cx_state = BUS_FREE;
	
	Delayms(100);
	
	_I2Cx_state = BUS_BUSY;
	BMP180_readCalibrationData();
	_I2Cx_state = BUS_FREE;
	
	_I2Cx_state = BUS_BUSY;
	deviceID = BMP180_readID();
	_I2Cx_state = BUS_FREE;
	
	if (deviceID == 0x55){
		TM_USART_Puts(USART1, "BMP INIT SUCCESS\n");
		return GUI_INIT_OK;
	}
	else {
		TM_USART_Puts(USART1, "BMP INIT FAILED\n");
		return GUI_INIT_FAILED;
	}
}

/*
	GUI init
*/
void GUI_InitMenu(void){
	hButtons[0] = BUTTON_CreateEx(10, 260, 70, 50, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON1);
	hButtons[1] = BUTTON_CreateEx(85, 260, 70, 50, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON2);
	hButtons[2] = BUTTON_CreateEx(160, 260, 70, 50, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON3);
	
	setFonts(0, 3, &GUI_Font13HB_ASCII);

	BUTTON_SetText(hButtons[0], "RADIO");
	BUTTON_SetText(hButtons[1], "CLOCK");
	BUTTON_SetText(hButtons[2], "ALARM");
	
	BUTTON_SetBkColor(hButtons[0], BUTTON_CI_UNPRESSED, GUI_DARKGREEN);
	BUTTON_SetBkColor(hButtons[0], BUTTON_CI_PRESSED, GUI_GREEN);
	
	BUTTON_SetBkColor(hButtons[2], BUTTON_CI_UNPRESSED, GUI_DARKRED);
	BUTTON_SetBkColor(hButtons[2], BUTTON_CI_PRESSED, GUI_RED);

	GUI_Exec();
}

/*
	GUI screen loops
*/

/*
	RADIO GUI loop
*/
uint16_t GUI_StartScreenRadio(void){
	int keyFlag = 0;
	uint8_t mute = TEA5767_Get_Mute();
	char buffer[50] = "";
	
	PROGBAR_Handle hProgbar = PROGBAR_CreateEx(10, 75, 219, 30, 0, WM_CF_SHOW, 0, GUI_ID_PROGBAR0);
	PROGBAR_SetFont(hProgbar, &GUI_Font8x16);
	PROGBAR_SetText(hProgbar, "");

	hButtons[3] = BUTTON_CreateEx(10, 120, 70, 50, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON4); // <
	hButtons[4] = BUTTON_CreateEx(85, 120, 70, 50, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON5); // AM/FM
	hButtons[5] = BUTTON_CreateEx(160, 120, 70, 50, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON6); // >
	hButtons[6] = BUTTON_CreateEx(85, 180, 70, 50, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON7); // MUTE
	
	setFonts(3, 6, &GUI_Font13HB_ASCII);

	BUTTON_SetText(hButtons[3], "<");
	BUTTON_SetText(hButtons[4], "SCAN");
	BUTTON_SetText(hButtons[5], ">");
	BUTTON_SetText(hButtons[6], "MUTE");
	GUI_Exec();
	
	while(keyFlag != CLOCK_KEY && keyFlag != ALARM_KEY){
		keyFlag = GUI_GetKey();
		
		if (mute == TEA5767_MUTE_ON) sprintf(buffer, "%.2f FM \nMUTE ON", _freq);
		else sprintf(buffer, "%.2f FM \n       ", _freq);
		TM_ILI9341_Puts(10, 5, buffer, &TM_Font_16x26, ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK);
		PROGBAR_SetValue(hProgbar, 100 - (107.6 - _freq) * 5);
		GUI_Exec();
		
		switch(keyFlag){
			case GUI_ID_BUTTON4:
				// lower freqency
				_freq = _freq - 0.1;
				_freq = floor(_freq * 10 + 0.5) / 10;
				if (_freq < 87.6) _freq = 107.6;
				TEA5767_Flags = TEA5767_SET_FREQUENCY;
				break;
			
			case GUI_ID_BUTTON5:
				// scan up			
				TEA5767_Flags = TEA5767_SCAN;
				break;
			
			case GUI_ID_BUTTON6:
				// increase frequency
				_freq = _freq + 0.1;
				_freq = floor(_freq * 10 + 0.5) / 10;
				if (_freq > 107.6) _freq = 87.6;
				TEA5767_Flags = TEA5767_SET_FREQUENCY;					
				break;
			
			case GUI_ID_BUTTON7:
				// mute
				if (mute == TEA5767_MUTE_ON) {
					TEA5767_Flags = TEA5767_MUTE_OFF;
					mute = TEA5767_MUTE_OFF;
				}
				else {
					TEA5767_Flags = TEA5767_MUTE_ON;
					mute = TEA5767_MUTE_ON;
				}
				break;
			default:
				break;
		}		
	}
	PROGBAR_Delete(hProgbar);
	deleteButtons(3, 6);
	GUI_ClearRect(10, 5, 230, 260);
	
	return keyFlag;
}

/*
	CLOCK GUI loop
*/
uint16_t GUI_StartScreenClock(){
	const uint8_t TEMP_MEAS_DELAY_MS = 10;
	const uint8_t PRESS_MEAS_DELAY_MS = 10;
	const uint8_t MID_MEAS_DELAY = 25;
	int keyFlag = 0;
	double modf_temp;
	char buffer[50] = "";
	
	PROGBAR_Handle hProgbar_temp = PROGBAR_CreateEx(10, 75, 219, 30, 0, WM_CF_SHOW, 0, GUI_ID_PROGBAR0);
	PROGBAR_Handle hProgbar_press = PROGBAR_CreateEx(10, 110, 219, 30, 0, WM_CF_SHOW, 0, GUI_ID_PROGBAR0);
	PROGBAR_SetFont(hProgbar_temp, &GUI_Font8x16);
	PROGBAR_SetFont(hProgbar_press, &GUI_Font8x16);
	
	
	while(keyFlag != RADIO_KEY && keyFlag != CLOCK_KEY && keyFlag != ALARM_KEY){
	//while(keyFlag != RADIO_KEY && keyFlag != SETTING_KEY && keyFlag != ALARM_KEY){
		keyFlag = GUI_GetKey();

		// Temperature measurement
		Delayms(MID_MEAS_DELAY);
		_I2Cx_state = BUS_BUSY;
		BMP180_startMeasurement(BMP180_M_TEMP);
		_I2Cx_state = BUS_FREE;
		Delayms(TEMP_MEAS_DELAY_MS);
		_I2Cx_state = BUS_BUSY;
		_temp = BMP180_readResult();
		_I2Cx_state = BUS_FREE;
		// Pressure
		Delayms(MID_MEAS_DELAY);
		_I2Cx_state = BUS_BUSY;
		BMP180_startMeasurement(BMP180_M_PRESS_OSS_0);
		_I2Cx_state = BUS_FREE;
		Delayms(PRESS_MEAS_DELAY_MS);
		_I2Cx_state = BUS_BUSY;
		_press = BMP180_readResult();
		_I2Cx_state = BUS_FREE;
		
		// show date, time, temperature
		sprintf(buffer, "%02d.%02d.%04d\n%02d:%02d:%02d",
			Time.date,
			Time.month,
			Time.year + 2000,
			Time.hours,
			Time.minutes,
			Time.seconds
		);
		TM_ILI9341_Puts(10, 5, buffer, &TM_Font_16x26, ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK);		

		sprintf(buffer, "%02d.%d [C]", _temp / 10, _temp % 10);

		PROGBAR_SetText(hProgbar_temp, buffer);
		PROGBAR_SetValue(hProgbar_temp, _temp * 10 / 65);
		
		sprintf(buffer, "%04d.%02d [hPa]", _press / 100, _press % 100);
		
		PROGBAR_SetText(hProgbar_press, buffer);
		PROGBAR_SetValue(hProgbar_press, (_press - 30000) / 800);
		
		GUI_Exec();
	}
	PROGBAR_Delete(hProgbar_temp);
	PROGBAR_Delete(hProgbar_press);
	GUI_ClearRect(10, 5, 230, 260 + 230 + 230);
	
	if(keyFlag == CLOCK_KEY) keyFlag = SETTING_KEY;
	
	return keyFlag;
}

/*
	CLOCK SET GUI loop
*/
uint16_t GUI_StartScreenClockSetting(){
	int keyFlag = 0;
	char buffer[50] = "";
	TM_RTC_Time_t SetTime = Time;
	
	hButtons[3] = BUTTON_CreateEx(50, 4, 40, 30, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON4); // H+
	hButtons[4] = BUTTON_CreateEx(50, 70, 40, 30, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON5); // H-
	hButtons[5] = BUTTON_CreateEx(100, 4, 40, 30, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON6); // M+
	hButtons[6] = BUTTON_CreateEx(100, 70, 40, 30, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON7); // M-
	hButtons[7] = BUTTON_CreateEx(150, 4, 40, 30, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON8); // M+
	hButtons[8] = BUTTON_CreateEx(150, 70, 40, 30, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON9); // M-
	
	hButtons[9] = BUTTON_CreateEx(40, 105, 40, 30, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON10); // D+
	hButtons[10] = BUTTON_CreateEx(40, 171, 40, 30, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON11); // D-
	hButtons[11] = BUTTON_CreateEx(90, 105, 40, 30, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON12); // M+
	hButtons[12] = BUTTON_CreateEx(90, 171, 40, 30, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON13); // M-
	hButtons[13] = BUTTON_CreateEx(140, 105, 60, 30, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON14); // Y+
	hButtons[14] = BUTTON_CreateEx(140, 171, 60, 30, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON15); // Y-

	hButtons[15] = BUTTON_CreateEx(85, 210, 70, 40, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON16); // Update		

	setFonts(3, 15, &GUI_Font13HB_ASCII);

	BUTTON_SetText(hButtons[3], "+");
	BUTTON_SetText(hButtons[4], "-");
	BUTTON_SetText(hButtons[5], "+");
	BUTTON_SetText(hButtons[6], "-");
	BUTTON_SetText(hButtons[7], "+");
	BUTTON_SetText(hButtons[8], "-");			

	BUTTON_SetText(hButtons[9], "+");
	BUTTON_SetText(hButtons[10], "-");
	BUTTON_SetText(hButtons[11], "+");
	BUTTON_SetText(hButtons[12], "-");
	BUTTON_SetText(hButtons[13], "+");
	BUTTON_SetText(hButtons[14], "-");
	
	BUTTON_SetText(hButtons[15], "Save");

	GUI_Exec();
	
	while(keyFlag != ALARM_KEY && keyFlag != CLOCK_KEY && keyFlag != ALARM_KEY){
		keyFlag = GUI_GetKey();
		
		sprintf(buffer, "%02d:%02d:%02d",
			SetTime.hours,
			SetTime.minutes,
			SetTime.seconds
		);
		TM_ILI9341_Puts(56, 40, buffer, &TM_Font_16x26, ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK);
	
		sprintf(buffer, "%02d-%02d-%04d",
			SetTime.date,
			SetTime.month,
			SetTime.year + 2000
		);
		TM_ILI9341_Puts(40, 142, buffer, &TM_Font_16x26, ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK);
	
		switch(keyFlag){
		case GUI_ID_BUTTON4:
			// hour++
			if (SetTime.hours < 23) SetTime.hours++;
			else SetTime.hours = 0;
			break;
		
		case GUI_ID_BUTTON5:
			// hour--
			if (SetTime.hours > 0) SetTime.hours--;
			else SetTime.hours = 23;
			break;
		
		case GUI_ID_BUTTON6:
			// minute++
			if (SetTime.minutes < 59) SetTime.minutes++;
			else SetTime.minutes = 0;
			break;
		
		case GUI_ID_BUTTON7:
			// minute--
			if (SetTime.minutes > 0) SetTime.minutes--;
			else SetTime.minutes = 59;
			break;
		
		case GUI_ID_BUTTON8:
			// minute++
			if (SetTime.seconds < 59) SetTime.seconds++;
			else SetTime.seconds = 0;
			break;
		
		case GUI_ID_BUTTON9:
			// minute--
			if (SetTime.seconds > 0) SetTime.seconds--;
			else SetTime.seconds = 59;
			break;

		case GUI_ID_BUTTON10:
			// day++
			if (SetTime.date < 31) SetTime.date++;
			else SetTime.date = 1;
			break;
		
		case GUI_ID_BUTTON11:
			// day--
			if (SetTime.date > 1) SetTime.date--;
			break;

		case GUI_ID_BUTTON12:
			// month++
			if (SetTime.month < 12) SetTime.month++;
			else SetTime.month = 1;
			break;

		case GUI_ID_BUTTON13:
			// month--
			if (SetTime.month > 1) SetTime.month--;
			else SetTime.month = 12;
			break;

		case GUI_ID_BUTTON14:
			// year++
			SetTime.year++;
			break;

		case GUI_ID_BUTTON15:
			// year--
			if (SetTime.year > 1) SetTime.year--;
			break;

		case GUI_ID_BUTTON16:
			// update
			TM_RTC_SetDateTime(&SetTime, TM_RTC_Format_BIN);
			break;	
		
		default:
			break;
		}		
	}
	
	deleteButtons(3, 15);
	GUI_ClearRect(10, 5, 230, 260);
	
	return keyFlag;
}

/*
	ALARM GUI loop
*/
uint16_t GUI_StartScreenAlarm(void){
	int keyFlag = 0;


	while(keyFlag != RADIO_KEY && keyFlag != CLOCK_KEY){
		keyFlag = GUI_GetKey();
		
	}
	
	return keyFlag;
}

/* User handler for 1ms interrupts */
void TM_DELAY_1msHandler(void) {
	/* Call periodically each 1ms */
	if (_I2Cx_state == BUS_FREE)
		TM_EMWIN_UpdateTouch();
	
	if (TEA5767_Flags == TEA5767_SET_FREQUENCY) TEA5767_Set_Frequency(_freq);
	if (TEA5767_Flags == TEA5767_SCAN) {TEA5767_Search_Up(); _freq = TEA5767_Get_Frequency();}
	if (TEA5767_Flags == TEA5767_MUTE_ON) TEA5767_Mute(TEA5767_MUTE_ON);
	if (TEA5767_Flags == TEA5767_MUTE_OFF) TEA5767_Mute(TEA5767_MUTE_OFF);
//	if (BMP180_Flags == BMP180_INIT) BMP180_initialize();
//	if (BMP180_Flags == BMP180_MEASURE_TEMPERATURE) BMP180_startMeasurement(BMP180_M_TEMP); 
//	if (BMP180_Flags == BMP180_MEASURE_PRESSURE) BMP180_startMeasurement(BMP180_M_TEMP);
//	if (BMP180_Flags == BMP180_READ_RESULT_TEMPERATURE) _temp = BMP180_readResult();
//	if (BMP180_Flags == BMP180_READ_RESULT_PRESSURE) _press = BMP180_readResult();
		
	TEA5767_Flags = TEA5767_NO_FLAG;
	BMP180_Flags = BMP180_NO_FLAG;
}

/* Custom request handler function */
/* Called on wakeup interrupt */
void TM_RTC_RequestHandler() {
	/* Get time */
	char buf[50];
	TM_RTC_GetDateTime(&Time, TM_RTC_Format_BIN);
	
	/* Format time */
	sprintf(buf, "%02d.%02d.%04d %02d:%02d:%02d  %f %d/%d  Unix: %u\n",
				Time.date,
				Time.month,
				Time.year + 2000,
				Time.hours,
				Time.minutes,
				Time.seconds,
				_freq,
				_temp,
				_press,
				Time.unix
	);
	
	/* Send to USART */
	TM_USART_Puts(USART1, buf);
	
	/* Toggle LED */
	TM_DISCO_LedToggle(LED_RED | LED_GREEN);
}

/* Custom request handler function */
/* Called on alarm A interrupt */
void TM_RTC_AlarmAHandler(void) {
	/* Show user to USART */
	TM_USART_Puts(USART1, "Alarm A triggered\n");
	
	/* Disable Alarm so it will not trigger next week at the same time */
	//TM_RTC_DisableAlarm(TM_RTC_Alarm_A);
}

/* Custom request handler function */
/* Called on alarm B interrupt */
void TM_RTC_AlarmBHandler(void) {
	/* Show user to USART */
	TM_USART_Puts(USART1, "Alarm B triggered\n");
	
	/* Disable Alarm so it will not trigger next month at the same date and time */
	//TM_RTC_DisableAlarm(TM_RTC_Alarm_A);
}
