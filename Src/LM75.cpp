extern "C"
{
#include "main.h"
}

#include "i2c.h"
#include "stm32f3xx_hal_i2c.h"
#include "LM75.h"

//-------------------------------------------------------------------------------

TempI2C_LM75::TempI2C_LM75(I2C_HandleTypeDef * hi2c,uint8_t i2c_addr)
{
	// on STM32, I2C addresses have to be shifted 1 bit left to allow for hardware insertion of the r/w bit as the MSb
	uint8_t _i2cAddr = (i2c_addr << 1);
   
	int status;
	//status = HAL_I2C_Master_Receive(hi2c, (uint16_t)_i2cAddr<<1, &_shadow, 1, 10);
	status = HAL_I2C_IsDeviceReady(hi2c, (uint16_t)_i2cAddr, 3, 1000); 
	
	if (status != HAL_OK) {
		m_u16I2CAddr = 0; // does not answer
	} else {
		m_u16I2CAddr = _i2cAddr;
		m_hi2c = hi2c;
		bool verifyShutdown;
		setShutdown(0);
		verifyShutdown=getShutdown();
	}
}

//-------------------------------------------------------------------------------
float TempI2C_LM75::getTemp()
{
	union {
		unsigned short tempX;
		short tempS;
	} temperature;

	temperature.tempX = getReg(temp_reg);
	return (temperature.tempS / 256.0F);
}

//-------------------------------------------------------------------------------
unsigned short TempI2C_LM75::getReg(LM75Register reg)
{
	unsigned short retVal = 0;
	bool regAlreadySet = (reg == previousReg);

	if (m_u16I2CAddr) {
		int status;
		uint8_t data[2];
		if (reg == config_reg) {
			if (regAlreadySet) {
				status = HAL_I2C_Master_Receive(m_hi2c, m_u16I2CAddr, &data[0], 1, 1000);
			} else {
				status = HAL_I2C_Mem_Read(m_hi2c, m_u16I2CAddr, reg, I2C_MEMADD_SIZE_8BIT, &data[0], 1, 1000);
			}
			retVal = data[0];  
		} else {
			if (regAlreadySet) {
				status = HAL_I2C_Master_Receive(m_hi2c, m_u16I2CAddr, &data[0], 2, 1000);
			} else {
				status = HAL_I2C_Mem_Read(m_hi2c, m_u16I2CAddr, reg, I2C_MEMADD_SIZE_8BIT, &data[0], 2, 1000);
			}
			retVal = data[0]<<8 | data[1];  
		}
		previousReg = reg;
	}
	return (retVal);
}
//-------------------------------------------------------------------------------
void TempI2C_LM75::setReg(LM75Register reg, unsigned newValue)
{
	int status = HAL_ERROR;

	if (m_u16I2CAddr) {
		// Only write HIGH the values of the ports that have been initialised as
		// outputs updating the output shadow of the device
		uint8_t data[3];
		short length;
		
		data[0] = newValue & 0xFF;
		if (reg == config_reg) {
			data[0] = newValue;

			status = HAL_I2C_Mem_Write(m_hi2c, m_u16I2CAddr, reg, I2C_MEMADD_SIZE_8BIT, &data[0], 1, 1000);
		} else {
			length = 1;
			data[1] = (newValue & 0xFF00) >> 8;

			status = HAL_I2C_Mem_Write(m_hi2c, m_u16I2CAddr, reg, I2C_MEMADD_SIZE_8BIT, &data[0], 2, 1000);
		}
		previousReg = reg;
	}
	//return ((status == HAL_OK)); // HAL_OK is 0 as well
}

//-------------------------------------------------------------------------------
void TempI2C_LM75::setTHyst(float newTHyst)
{
	setReg(THyst_reg, int(newTHyst * 256));
}

//-------------------------------------------------------------------------------
void TempI2C_LM75::setTOS(float newTOS)
{
	setReg(TOS_reg, int(newTOS * 256));
}

//-------------------------------------------------------------------------------
float TempI2C_LM75::getTHyst(void)
{
	return (int(getReg(THyst_reg)) / 256.0F);
}

//-------------------------------------------------------------------------------
float TempI2C_LM75::getTOS(void)
{
	return (int(getReg(TOS_reg)) / 256.0F);
}

//-------------------------------------------------------------------------------
TempI2C_LM75::ThermostatMode TempI2C_LM75::getThermostatMode()
{
	CfgRegister regv;

	regv.mbyte = getReg(config_reg);

	return (ThermostatMode(regv.mbits.thermostat_mode));
}

//-------------------------------------------------------------------------------
void TempI2C_LM75::setThermostatMode(TempI2C_LM75::ThermostatMode newMode)
{
	CfgRegister regv;

	regv.mbyte = getReg(config_reg);
	regv.mbits.thermostat_mode = newMode;
	regv.mbits.reserved = 0;
	setReg(config_reg, unsigned(regv.mbyte));
}

//-------------------------------------------------------------------------------
TempI2C_LM75::ThermostatFaultTolerance TempI2C_LM75::getThermostatFaultTolerance()
{
	CfgRegister regv;

	regv.mbyte = getReg(config_reg);

	return (ThermostatFaultTolerance(regv.mbits.thermostat_fault_tolerance));
}

//-------------------------------------------------------------------------------
void TempI2C_LM75::setThermostatFaultTolerance(ThermostatFaultTolerance newFaultTolerance)
{
	CfgRegister regv;

	regv.mbyte = getReg(config_reg);
	regv.mbits.thermostat_fault_tolerance = newFaultTolerance;
	regv.mbits.reserved = 0;
	setReg(config_reg, unsigned(regv.mbyte));
}


bool TempI2C_LM75::getShutdown()
{
	CfgRegister regv;

	regv.mbyte = getReg(config_reg);

	return (regv.mbits.shutdown);
}
void TempI2C_LM75::setShutdown(bool newShutdown)
{
	CfgRegister regv;

	regv.mbyte = getReg(config_reg);
	regv.mbits.shutdown = newShutdown;
	regv.mbits.reserved = 0;
	setReg(config_reg, unsigned(regv.mbyte));
}
//-------------------------------------------------------------------------------
TempI2C_LM75::OSPolarity TempI2C_LM75::getOSPolarity()
{
	CfgRegister regv;

	regv.mbyte = getReg(config_reg);

	return (OSPolarity(regv.mbits.thermostat_output_polarity));
}

//-------------------------------------------------------------------------------
void TempI2C_LM75::setOSPolarity(OSPolarity newOSPolarity)
{
	CfgRegister regv;

	regv.mbyte = getReg(config_reg);
	regv.mbits.thermostat_output_polarity = newOSPolarity;
	regv.mbits.reserved = 0;
	setReg(config_reg, unsigned(regv.mbyte));
}
