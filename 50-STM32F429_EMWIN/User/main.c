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

int main(void) {
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
		/* Initialization error */
		while (1) {
			TM_DISCO_LedToggle(LED_RED);	
			Delayms(100);
		}
	}
	
	// SCL = PA8, SDA = PC9
	if (GUI_TEA5767_Init(I2C3) != GUI_INIT_OK){
		while (1) {
			TM_DISCO_LedToggle(LED_RED);	
			Delayms(100);
		}
	}
	if (GUI_BMP180_Init(I2C3) != GUI_INIT_OK){
		while (1) {
			TM_DISCO_LedToggle(LED_RED);	
			Delayms(100);
		}		
	}
	
	TM_ILI9341_Rotate(TM_ILI9341_Orientation_Portrait_2);
	
	TM_USART_Init(USART1, TM_USART_PinsPack_1, 115200); // TX: PA9, RX: PA10
	
	/* Initialize RTC with internal 32768Hz clock */
	/* It's not very accurate */
	if (!TM_RTC_Init(TM_RTC_ClockSource_Internal)) {
		/* RTC was first time initialized */
		/* Do your stuff here */
		/* eg. set default time */
	}
	
	TM_RTC_Interrupts(TM_RTC_Int_1s);
	
	GUI_InitMenu();
	uint16_t sceneFlag = CLOCK;
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
