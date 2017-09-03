#ifndef gui_screens
#define gui_screens

#include "stm32f4xx.h"

typedef enum {
	GUI_INIT_FAILED,
	GUI_INIT_OK
}GUI_InitResult;

/**
	@brief Initialize driver - check i2c interface validity
	@note Requires initialized i2c interface stuct
	@retval Retcode GUI_INIT_OK or GUI_INIT_FAILED
*/
GUI_InitResult GUI_TEA5767_Init(I2C_TypeDef* I2Cx);

/**
	@brief Initialize driver - check i2c interface validity
	@note Requires initialized i2c interface stuct
	@retval Retcode GUI_INIT_OK or GUI_INIT_FAILED
*/
GUI_InitResult GUI_BMP180_Init(I2C_TypeDef* I2Cx);

/**
	@brief Initialize menu layout
	@note Requires initialized emWin
	@retval Return value of last GUI_GetKey() call
*/
void GUI_InitMenu(void);

/**
	@brief Display radio functionality
	@note Requires initialized emWin and menu
	@retval Return value of last GUI_GetKey() call
*/
uint16_t GUI_StartScreenRadio(void);

/**
	@brief Display clockface screen
	@note Requires initialized emWin and menu
	@retval Return value of last GUI_GetKey() call
*/
uint16_t GUI_StartScreenClock(void);

/**
	@brief Display time setting functionality 
	@note Requires initialized emWin and menu
	@retval Return value of last GUI_GetKey() call
*/
uint16_t GUI_StartScreenClockSetting(void);

/**
	@brief Display alarm setting functionality
	@note Requires initialized emWin and menu
	@retval Return value of last GUI_GetKey() call
*/
uint16_t GUI_StartScreenAlarm(void);

#endif
