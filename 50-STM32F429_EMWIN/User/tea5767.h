#ifndef tea5767_h
#define tea5767_h

#include "stm32f4xx.h"

typedef enum {
	TEA5767_INIT_FAILED,
	TEA5767_INIT_OK,
	TEA5767_MONO,
	TEA5767_STEREO,
	TEA5767_MUTE_ON,
	TEA5767_MUTE_OFF
} TEA5767_retcodes;

typedef struct TEA5767_I2C_interface {
	void (*readBytesNoRegister)(uint8_t addr, uint8_t *data, uint16_t count);
	void (*writeBytesNoRegister)(uint8_t addr, uint8_t *data, uint16_t count);
	uint8_t (*isConnected)(uint8_t address);
} TEA5767_I2C_interface;

/**
	@brief Get pointer to internal TEA5767 driver i2c bus interface structure
	@retval Pointer to i2c interface struct allowing for init with i2c library
*/
TEA5767_I2C_interface* TEA5767_getI2CInterfacePtr(void);

/**
	@brief Initialize driver - check i2c interface validity
	@note Requires initialized i2c interface stuct
	@retval Retcode TEA5767_INIT_OK or TEA5767_INIT_FAILED
*/
uint8_t TEA5767_Init(void);

/**
	@brief Read TEA5767 RSSI
	@retval Signal level
*/
uint8_t TEA5767_Signal_Level(void);

/**
	@brief Read TEA5767 playback mode
	@retval Retcode TEA5767_MONO or TEA5767_STEREO
*/
uint8_t TEA5767_Stereo(void);

/**
	@brief Read TEA5767 mute status
	@retval Retcode TEA5767_MUTE_ON or TEA5767_MUTE_OFF
*/
uint8_t TEA5767_Get_Mute(void);

/**
	@brief Set TEA5767 radio frequency
	@retval None
*/
void TEA5767_Set_Frequency(float freq);

/**
	@brief Get TEA5767 radio frequency
	@retval radio frequency
*/
float TEA5767_Get_Frequency(void);

/**
	@brief Start TEA5767 search for next aviable frequency
	@retval None
*/
void TEA5767_Search_Up(void);

/**
	@brief Mute TEA5767 output
	@retval None
*/
void TEA5767_Mute(uint8_t mute);

#endif
