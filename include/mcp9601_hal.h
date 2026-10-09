#ifndef MCP9601_HAL_H
#define MCP9601_HAL_H

#include "hardware/i2c.h"
#include "mcp9601_reg.h"
#include "common_defs.h"
#include <stdint.h>

typedef enum {
    RES_18B,
    RES_16B,
    RES_14B,
    RES_12B
} adc_res_t;

typedef enum {
    HIGH_RES,
    LOW_RES
} tcold_res_t;

typedef struct {
    i2c_inst_t* i2c;
    uint8_t addr;
    adc_res_t adc_res;
    tcold_res_t cold_res;
    uint8_t data_index;
} mcp9601_t;

typedef struct {
    uint8_t index;
    float tempHot[MCP960X_DATA_NUM];
    float tempCold[MCP960X_DATA_NUM];
} mcp960xData_t;

uint8_t mcp9601_check_available(i2c_inst_t* i2c, uint8_t addr, uint8_t* rxdata);
uint8_t mcp9601_device_scan(i2c_inst_t* i2c);
uint8_t mcp9601_set_device_config(mcp9601_t *dev, tcold_res_t tcoldres, adc_res_t adcres);
uint8_t mcp9601_init_device(mcp9601_t *dev);
i2c_err_t mcp9601_read_tCold_reg(mcp9601_t *dev, uint8_t *buf, size_t buflen);
i2c_err_t mcp9601_read_tHot_reg(mcp9601_t *dev, uint8_t *buf, size_t buflen);
i2c_err_t mcp9601_read_tDelta_reg(mcp9601_t *dev, uint8_t *buf, size_t buflen);
i2c_err_t mcp9601_read_alltemp_reg(mcp9601_t *dev, uint8_t* buf, size_t buflen);
float mcp9601_convert_to_temp(uint8_t* buf);

#endif