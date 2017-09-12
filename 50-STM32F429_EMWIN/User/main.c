/**
 *	Keil project example for emWin
 *
 *	Before you start, select your target, on the right of the "Load" button
 *
 *	@author		Tilen Majerle
 *	@email		tilen@majerle.eu
 *	@website	http://stm32f4-discovery.com
 *	@ide		Keil uVision 5
 *	@packs		STM32F4xx Keil packs version 2.2.0 or greater required
 *	@stdperiph	STM32F4xx Standard peripheral drivers version 1.4.0 or greater required
 */
/* Include core modules */
#include "stm32f4xx.h"
#include "tm_stm32f4_delay.h"
#include "tm_stm32f4_disco.h"
#include "tm_stm32f4_emwin.h"
#include "tm_stm32f4_rtc.h"
#include "tm_stm32f4_usart.h"
#include "GUI_screens.h"
#include "defines.h"

#include "bmp180.h"
#include "tea5767.h"

static I2C_TypeDef* _I2Cx = I2C3; // SCL = PA8, SDA = PC9

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

// -------------------------------------------------------------------------------------- //

/*
	MAIN
*/
int main(void) {
	uint16_t sceneFlag;
	TEA5767_I2C_interface* tea5767i2cInterfacePtr;
	BMP180_I2C_interface * bmp180i2cInterfacePtr;	
	
	enum scene {
	RADIO = GUI_ID_BUTTON1, 
	CLOCK = GUI_ID_BUTTON2, 
	ALARM = GUI_ID_BUTTON3, 
	CLOCK_SETTING = GUI_ID_BUTTON4
	};
		
	SystemInit();
	TM_DELAY_Init();
	TM_DISCO_LedInit();
		
	/* Initialize emWin */
	if (TM_EMWIN_Init() != TM_EMWIN_Result_Ok) {
		while (1) {
			TM_DISCO_LedToggle(LED_RED);	
			Delayms(200);
		}
	}
	
	// Init TEA5767 I2C interface 
	tea5767i2cInterfacePtr = TEA5767_getI2CInterfacePtr();
	tea5767i2cInterfacePtr->writeBytesNoRegister = tea5767compliant_writeBytesNoRegister;
	tea5767i2cInterfacePtr->readBytesNoRegister = tea5767compliant_readBytesNoRegister;
	tea5767i2cInterfacePtr->isConnected = tea5767compliant_isConnected;
	// Init TEA5767
	if (GUI_TEA5767_Init(I2C3) != GUI_INIT_OK){
		while (1) {
			TM_DISCO_LedToggle(LED_RED);	
			Delayms(1000);
		}
	}
	
	// Init BMP180 I2C interface 
	bmp180i2cInterfacePtr = BMP180_getI2CInterfacePtr();
	bmp180i2cInterfacePtr->readByte = bmp180compliant_readByte;
	bmp180i2cInterfacePtr->readBytes = bmp180compliant_readBytes;
	bmp180i2cInterfacePtr->writeByte = bmp180compliant_writeByte;
	bmp180i2cInterfacePtr->writeBytes = bmp180compliant_writeBytes;
	// Init BMP180
	if (GUI_BMP180_Init(I2C3) != GUI_INIT_OK){
		while (1) {
			TM_DISCO_LedToggle(LED_RED);	
			Delayms(2000);
		}		
	}
	
	TM_ILI9341_Rotate(TM_ILI9341_Orientation_Portrait_2);
	
	TM_USART_Init(USART1, TM_USART_PinsPack_1, 115200); // TX: PA9, RX: PA10
	
	if (!TM_RTC_Init(TM_RTC_ClockSource_Internal)) {}
	
	TM_RTC_Interrupts(TM_RTC_Int_1s);
	
	GUI_InitMenu();
	sceneFlag = CLOCK;
	while (1) {
		switch (sceneFlag){
			case RADIO:
				sceneFlag = GUI_StartScreenRadio();
			break;
			
			case CLOCK:
				sceneFlag = GUI_StartScreenClock();
			break;
			
			case ALARM:
				sceneFlag = GUI_StartScreenAlarm();
			break;
			
			case CLOCK_SETTING:
				sceneFlag = GUI_StartScreenClockSetting();
			break;
			
			default:
				break;
		}
	}
}
