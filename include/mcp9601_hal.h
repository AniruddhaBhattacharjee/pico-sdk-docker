#ifndef MCP9601_HAL_H
#define MCP9601_HAL_H

#include "hardware/i2c.h"
#include "mcp9601_reg.h"
#include "common_defs.h"
#include <stdint.h>

typedef struct {
    i2c_inst_t* i2c;
    uint8_t addr;
}mcp9601_t;

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

/*
typedef enum {
    WR_ERROR,
    RD_ERROR_1,
    RD_ERROR_2,
    RD_ERROR_3,
    ARG_ERROR,
    NO_ERROR
} mcp_i2c_err_t;
*/

uint8_t mcp9601_check_available(i2c_inst_t* i2c, uint8_t addr, uint8_t* rxdata);
uint8_t mcp9601_device_scan(i2c_inst_t* i2c);
uint8_t mcp9601_set_device_config(mcp9601_t *dev, tcold_res_t tcoldres, adc_res_t adcres);
i2c_err_t mcp9601_read_tCold_reg(mcp9601_t *dev, uint8_t *buf, size_t buflen);
i2c_err_t mcp9601_read_tHot_reg(mcp9601_t *dev, uint8_t *buf, size_t buflen);
i2c_err_t mcp9601_read_tDelta_reg(mcp9601_t *dev, uint8_t *buf, size_t buflen);
i2c_err_t mcp9601_read_alltemp_reg(mcp9601_t *dev, uint8_t* buf, size_t buflen);
float mcp9601_convert_to_temp(uint8_t* buf);

#endif