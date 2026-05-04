#ifndef INA228_H
#define INA228_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"
#include "ina228_reg.h"
//#include "i2c_if.h"
#include "common_defs.h"

typedef struct
{
    i2c_inst_t* i2c;
    uint8_t addr;
    float shunt_resistor;
    float current_lsb;// A per bit(calibration)

} ina228_t;

static int32_t read_s24(uint8_t *buf);
static uint16_t read_u16(uint8_t *buf);
i2c_err_t ina228_check_available(ina228_t *dev);
i2c_err_t ina228_init(ina228_t *dev);
i2c_err_t ina228_set_calibration(ina228_t *dev, float max_current, float shunt_resistor);
i2c_err_t ina228_read_bus_voltage(ina228_t *dev, float *voltage);
i2c_err_t ina228_read_shunt_voltage(ina228_t *dev, float *voltage);
i2c_err_t ina228_read_current(ina228_t *dev, float *current);
i2c_err_t ina228_read_power(ina228_t *dev, float *power);
i2c_err_t ina228_read_energy(ina228_t *dev, double *energy);

#endif
