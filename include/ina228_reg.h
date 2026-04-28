#ifndef INA228_REG_H
#define INA228_REG_H

#define INA228_I2C_ADDR_DEFAULT  0x40

// Registers Definitions
#define INA228_REG_CONFIG        0x00
#define INA228_REG_ADC_CONFIG    0x01
#define INA228_REG_SHUNT_CAL     0x02

#define INA228_REG_SHUNT_VOLT    0x04
#define INA228_REG_BUS_VOLT      0x05
#define INA228_REG_TEMP          0x06
#define INA228_REG_CURRENT       0x07
#define INA228_REG_POWER         0x08
#define INA228_REG_ENERGY        0x09

#endif
