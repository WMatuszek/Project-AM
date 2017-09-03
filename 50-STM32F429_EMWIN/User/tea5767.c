#include "tea5767.h"

enum TEA5767_registers{
	TEA5767_MUTE = 0x80,
	TEA5767_SEARCH = 0x40,
	TEA5767_SEARCH_UP = 0x80,
	TEA5767_SRCH_HIGH_LVL = 0x60,
	TEA5767_SRCH_MID_LVL = 0x40,
	TEA5767_SRCH_LOW_LVL = 0x20,
	TEA5767_HIGH_LO_INJECT = 0x10,
	TEA5767_FORCE_MONO = 0x08,
	TEA5767_MUTE_RIGHT = 0x04,
	TEA5767_MUTE_LEFT = 0x02,
	TEA5767_PORT1_HIGH = 0x01,
	TEA5767_PORT2_HIGH = 0x80,
	TEA5767_STDBY = 0x40,
	TEA5767_JAPAN_BAND = 0x20,
	TEA5767_XTAL_32768 = 0x10,
	TEA5767_SOFT_MUTE = 0x08,
	TEA5767_HIGH_CUT_CTRL = 0x04,
	TEA5767_ST_NOISE_CTL = 0x02,
	TEA5767_SRCH_IND = 0x01,
	TEA5767_PLLREF_ENABLE = 0x80,
	TEA5767_DEEMPH_75 = 0x40,
	TEA5767_READY_FLAG_MASK = 0x80,
	TEA5767_BAND_LIMIT_MASK = 0X40,
	TEA5767_STEREO_MASK = 0x80,
	TEA5767_ADC_LEVEL_MASK = 0xf0,
	TEA5767_CHIP_ID_MASK = 0x0f,
	TEA5767_RESERVED_MASK = 0xff,
	TEA5767_SEARCH_DIR_UP = 0x01,
	TEA5767_SEARCH_DIR_DOWN = 0x02,
	TEA5767_REGISTER_COUNT = 0x05,
	TEA5767_ADDRESS = 0xC0
};

struct TEA5767_CTRL_REGISTERS{
  unsigned int port1:1;
  unsigned int port2:1;
  unsigned int high_cut:1;
	unsigned int mute;
  unsigned int st_noise:1;
  unsigned int soft_mute:1;
  unsigned int japan_band:1;
  unsigned int deemph_75:1;
  unsigned int pllref:1;
  unsigned int xtal_freq;
	I2C_TypeDef* I2Cx;
} TEA5767_CTRL_REGISTERS;

uint8_t HILO = 0;

static TEA5767_I2C_interface i2c;

void TEA5767_I2C_Write(uint8_t data[]);
void TEA5767_I2C_Read(uint8_t* data);

uint8_t signal_level(unsigned char *buffer);
uint8_t is_stereo(unsigned char *buffer);
uint8_t band_limit_reached(unsigned char *buffer);
uint8_t hilo_optimal(uint16_t freq);
float current_frequency(unsigned char *buffer);
void set_frequency(uint8_t hilo, float freq, unsigned char *buffer);
void process_data(uint8_t* buffer, uint8_t* data);

TEA5767_I2C_interface* TEA5767_getI2CInterfacePtr(void){
	return &i2c;
}

uint8_t TEA5767_Init(void){
	if (i2c.isConnected(TEA5767_ADDRESS) != 0x01) return 0;
	
	TEA5767_CTRL_REGISTERS.port1 = 1;
  TEA5767_CTRL_REGISTERS.port2 = 1;
  TEA5767_CTRL_REGISTERS.high_cut = 1;
  TEA5767_CTRL_REGISTERS.st_noise = 1;
  TEA5767_CTRL_REGISTERS.soft_mute = 1;
  TEA5767_CTRL_REGISTERS.deemph_75 = 0;
  TEA5767_CTRL_REGISTERS.japan_band = 0;
  TEA5767_CTRL_REGISTERS.pllref = 0;
	return 1;
}

void TEA5767_Set_Frequency(float freq){
	uint8_t buffer[5] = "";
	HILO = hilo_optimal((unsigned long) (freq * 1000000));
	set_frequency(HILO, freq, buffer);
	TEA5767_I2C_Write(buffer);
}

float TEA5767_Get_Frequency(void){
	uint8_t data[5] = "";
	TEA5767_I2C_Read(data);
	return (current_frequency(data)/1000000);
}

void TEA5767_Search_Up(void){
	uint8_t data[5] = "";
	uint8_t buffer[5] = "";
	uint16_t div;
	
	TEA5767_I2C_Read(data);
	
	process_data(buffer, data);
	if (current_frequency(buffer) <= 107600000) div = (4 * (((current_frequency(buffer) + 98304) / 1000000) * 1000000 + 225000)) / 32768;
	else div = (4 * (87.6 * 1000 + 225)) / 32.768;

	buffer[0] = (div >> 8) & 0x3f;
  buffer[1] = div & 0xff;
	buffer[0] |= TEA5767_SEARCH;
	buffer[2] = 0;
	buffer[2] |= TEA5767_SEARCH_UP;
	buffer[2] |= TEA5767_SRCH_MID_LVL;
	buffer[2] |= TEA5767_HIGH_LO_INJECT;	
	
	TEA5767_I2C_Write(buffer);
	HILO = 1;
}

uint8_t TEA5767_Get_Mute(void){
	return TEA5767_CTRL_REGISTERS.mute;
}

uint8_t TEA5767_Signal_Level(void){
	uint8_t data[5] = "";
	TEA5767_I2C_Read(data);
	return signal_level(data);
}

uint8_t TEA5767_Stereo(void){
	uint8_t data[5] = "";
	TEA5767_I2C_Read(data);
	return is_stereo(data);
}

void TEA5767_Mute(uint8_t mute){
	uint8_t data[5] = "";
	uint8_t buffer[5] = "";
	
	TEA5767_CTRL_REGISTERS.mute = mute;
	
	TEA5767_I2C_Read(data);
	process_data(buffer, data);
	TEA5767_I2C_Write(buffer);
}

void TEA5767_I2C_Write(uint8_t data[]){
	i2c.writeBytesNoRegister(TEA5767_ADDRESS, data, TEA5767_REGISTER_COUNT);
}

void TEA5767_I2C_Read(uint8_t* data){
	i2c.readBytesNoRegister(TEA5767_ADDRESS, data, TEA5767_REGISTER_COUNT);
}

void set_frequency(uint8_t hilo, float freq, unsigned char *buffer){
	uint16_t div;
	
	buffer[2] |= TEA5767_PORT1_HIGH;
	
	if (hilo) buffer[2] |= TEA5767_HIGH_LO_INJECT;
	if (TEA5767_CTRL_REGISTERS.port2) buffer[3] |= TEA5767_PORT2_HIGH;
	if (TEA5767_CTRL_REGISTERS.high_cut) buffer[3] |= TEA5767_HIGH_CUT_CTRL;
	if (TEA5767_CTRL_REGISTERS.st_noise)	buffer[3] |= TEA5767_ST_NOISE_CTL;
  if (TEA5767_CTRL_REGISTERS.soft_mute) buffer[3] |= TEA5767_SOFT_MUTE;
	if (TEA5767_CTRL_REGISTERS.japan_band) buffer[3] |= TEA5767_JAPAN_BAND;

 	buffer[3] |= TEA5767_XTAL_32768;
	
	if (TEA5767_CTRL_REGISTERS.deemph_75) buffer[4] |= TEA5767_DEEMPH_75;
 	if (TEA5767_CTRL_REGISTERS.pllref)	buffer[4] |= TEA5767_PLLREF_ENABLE;
	
	if (hilo == 1){
		div = (4 * (freq * 1000 + 225)) / 32.768;
	}
	else
		div = (4 * (freq * 1000 - 225)) / 32.768;

	buffer[0] = (div >> 8) & 0x3f;	
	buffer[1] = div & 0xff;
	
	if (TEA5767_CTRL_REGISTERS.mute == TEA5767_MUTE_ON) buffer[0] |= TEA5767_MUTE;
}

uint8_t signal_level(unsigned char *buffer){
	uint8_t signal = ((buffer[3] & TEA5767_ADC_LEVEL_MASK) >> 4);
	return signal;
}
uint8_t is_stereo(unsigned char *buffer){
	uint8_t stereo = buffer[2] & TEA5767_STEREO_MASK;
	return stereo ? TEA5767_STEREO : TEA5767_MONO;
}

uint8_t band_limit_reached(unsigned char *buffer){
	return (buffer[0] & 0x40) ? 1 : 0;
}

float current_frequency(unsigned char *buffer){
	float freq;
	if (HILO) freq = (((buffer[0] & 0x3F) << 8) + buffer[1]) * 32768 / 4 - 225000;
	else freq = (((buffer[0] & 0x3F) << 8) + buffer[1]) * 32768 / 4 + 225000;
	return freq;
}

void process_data(uint8_t* buffer, uint8_t* data){
	buffer[2] |= TEA5767_PORT1_HIGH;
	
	if (HILO) buffer[2] |= TEA5767_HIGH_LO_INJECT;
	if (TEA5767_CTRL_REGISTERS.port2) buffer[3] |= TEA5767_PORT2_HIGH;
	if (TEA5767_CTRL_REGISTERS.high_cut) buffer[3] |= TEA5767_HIGH_CUT_CTRL;
	if (TEA5767_CTRL_REGISTERS.st_noise)	buffer[3] |= TEA5767_ST_NOISE_CTL;
  if (TEA5767_CTRL_REGISTERS.soft_mute) buffer[3] |= TEA5767_SOFT_MUTE;
	if (TEA5767_CTRL_REGISTERS.japan_band) buffer[3] |= TEA5767_JAPAN_BAND;

 	buffer[3] |= TEA5767_XTAL_32768;
	
	if (TEA5767_CTRL_REGISTERS.deemph_75) buffer[4] |= TEA5767_DEEMPH_75;
 	if (TEA5767_CTRL_REGISTERS.pllref)	buffer[4] |= TEA5767_PLLREF_ENABLE;
	
	buffer[0] = data[0] & 0x3f;
	buffer[1] = data[1];
	if (TEA5767_CTRL_REGISTERS.mute == TEA5767_MUTE_ON) buffer[0] |= TEA5767_MUTE;
}

uint8_t hilo_optimal(uint16_t freq){
	uint8_t signal_high = 0;
	uint8_t signal_low = 0;
	unsigned char buffer[5] = "";
	unsigned char buffer_2[5] = "";
	unsigned char data[5] = "";
	
	set_frequency(1, (double) (freq + 450000) / 1000000, buffer);
	TEA5767_I2C_Write(buffer);
	//delay 30
	
	TEA5767_I2C_Read(data);
	signal_high = signal_level(buffer);
	
	set_frequency(0, (double) (freq - 450000) / 1000000, buffer_2);
	TEA5767_I2C_Write(buffer_2);
	//delay 30
	
	TEA5767_I2C_Read(data);
	signal_low = signal_level(buffer);
	
	return (signal_high < signal_low) ? 1 : 0;
}



