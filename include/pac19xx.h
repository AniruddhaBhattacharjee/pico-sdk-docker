#ifndef PAC19XX_H
#define PAC19XX_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"
#include "common_defs.h"
#include "pac19xx_reg.h"

#define PAC19XX_NUM_CHANNELS 4
// PAC1954 CLICK Board uses 4 mOhm Shunt resistors
#define PAC19XX_SHUNT_RES (float)(0.004f)

typedef enum
{
    PAC_DEVICE_1944,
    PAC_DEVICE_1954
} pac19xx_device_type_t;

typedef struct
{
    i2c_inst_t *i2c;
    uint8_t addr;
    pac19xx_device_type_t type;
    float shunt_resistor[PAC19XX_NUM_CHANNELS];

} pac19xx_t;

bool pac19xx_init(pac19xx_t *dev);
i2c_err_t pac19xx_refresh(pac19xx_t *dev);
i2c_err_t pac19xx_read_bus_voltage(pac19xx_t *dev, int ch, float *voltage);
i2c_err_t pac19xx_read_shunt_voltage(pac19xx_t *dev, int ch, float *voltage);
i2c_err_t pac19xx_read_current(pac19xx_t *dev, int ch, float *current);
i2c_err_t pac19xx_read_power(pac19xx_t *dev, int ch, float *power);

#endif
