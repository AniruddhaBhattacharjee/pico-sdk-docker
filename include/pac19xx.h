#ifndef PAC19XX_H
#define PAC19XX_H

#include <stdint.h>
#include <stdbool.h>
#include "i2c_if.h"

#define PAC19XX_NUM_CHANNELS 4

typedef enum
{
    PAC_DEVICE_1944,
    PAC_DEVICE_1954
} pac19xx_device_type_t;

typedef struct
{
    i2c_device_t i2c;
    pac19xx_device_type_t type;
    float shunt_resistor[PAC19XX_NUM_CHANNELS];

} pac19xx_t;

bool pac19xx_init(pac19xx_t *dev);
bool pac19xx_refresh(pac19xx_t *dev);
bool pac19xx_read_bus_voltage(pac19xx_t *dev, int ch, float *voltage);
bool pac19xx_read_sense_voltage(pac19xx_t *dev, int ch, float *voltage);
bool pac19xx_read_current(pac19xx_t *dev, int ch, float *current);
bool pac19xx_read_power(pac19xx_t *dev, int ch, float *power);

#endif
