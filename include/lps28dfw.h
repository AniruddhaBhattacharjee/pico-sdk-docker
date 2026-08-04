#ifndef LPS28DFW_H
#define LPS28DFW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "hardware/i2c.h"

#include "common_defs.h"
#include "lps28dfw_reg.h"

/******************************************************************************
 * Number of stored samples
 ******************************************************************************/

#ifndef LPS28DFW_DATA_NUM
#define LPS28DFW_DATA_NUM 4
#endif

/******************************************************************************
 * Device Structure
 ******************************************************************************/

typedef struct{
    i2c_inst_t *i2c;
    uint8_t addr;
    uint8_t data_index;
} lps28dfw_t;

/******************************************************************************
 * Sensor Data Structure
 ******************************************************************************/

typedef struct{
    float pressure[LPS28DFW_DATA_NUM];
    float temperature[LPS28DFW_DATA_NUM];
} lps28dfwData_t;

/******************************************************************************
 * Configuration Structure
 ******************************************************************************/

typedef struct
{
    lps28dfw_odr_t odr;
    lps28dfw_lpf_t lpf;
    bool enable_bdu;
    bool auto_increment;
} lps28dfw_config_t;

/******************************************************************************
 * Public API
 ******************************************************************************/

/* Device */

int lps28dfw_check_available(lps28dfw_t *dev);
i2c_err_t lps28dfw_init(lps28dfw_t *dev);
i2c_err_t lps28dfw_soft_reset(lps28dfw_t *dev);
i2c_err_t lps28dfw_set_config(lps28dfw_t *dev,const lps28dfw_config_t *cfg);
i2c_err_t lps28dfw_set_odr(lps28dfw_t *dev,lps28dfw_odr_t odr);
int lps28dfw_enable_lowpass(lps28dfw_t *dev,lps28dfw_lpf_t mode);

/******************************************************************************
 * Register Access
 ******************************************************************************/

i2c_err_t lps28dfw_read_register(lps28dfw_t *dev,uint8_t reg,uint8_t *data,size_t len);
i2c_err_t lps28dfw_write_register(lps28dfw_t *dev,uint8_t reg,const uint8_t *data,size_t len);

/******************************************************************************
 * Raw Measurements
 ******************************************************************************/

i2c_err_t lps28dfw_read_pressure_raw(lps28dfw_t *dev,int32_t *raw_pressure);
i2c_err_t lps28dfw_read_temperature_raw(lps28dfw_t *dev,int16_t *raw_temperature);
i2c_err_t lps28dfw_read_raw(lps28dfw_t *dev,int32_t *raw_pressure,int16_t *raw_temperature);

/******************************************************************************
 * Floating Point Measurements
 ******************************************************************************/

i2c_err_t lps28dfw_read_pressure(lps28dfw_t *dev,float *pressure_hpa);
i2c_err_t lps28dfw_read_temperature(lps28dfw_t *dev,float *temperature_c);
i2c_err_t lps28dfw_read_all(lps28dfw_t *dev,float *pressure_hpa,float *temperature_c);
/******************************************************************************
 * Conversion Helpers
 ******************************************************************************/

float lps28dfw_convert_pressure(int32_t raw_pressure);
float lps28dfw_convert_temperature(int16_t raw_temperature);

/******************************************************************************
 * FIFO Functions
 ******************************************************************************/

i2c_err_t lps28dfw_fifo_set_mode(lps28dfw_t *dev, lps28dfw_fifo_mode_t mode);
int lps28dfw_fifo_get_level(lps28dfw_t *dev,uint8_t *level);
int lps28dfw_fifo_read_sample(lps28dfw_t *dev,float *pressure,float *temperature);
/******************************************************************************
 * Status
 ******************************************************************************/

i2c_err_t lps28dfw_get_status(lps28dfw_t *dev,uint8_t *status);
bool lps28dfw_pressure_ready(lps28dfw_t *dev);
bool lps28dfw_temperature_ready(lps28dfw_t *dev);

/******************************************************************************
 * Internal Helpers
 * Used only by lps28dfw.c
 ******************************************************************************/
int lps28dfw_read_reg(lps28dfw_t *dev,uint8_t reg,uint8_t *buf,size_t len);
int lps28dfw_write_reg(lps28dfw_t *dev,uint8_t reg,uint8_t value);

#ifdef __cplusplus
}
#endif

#endif