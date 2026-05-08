#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

#define MCP960X_DATA_NUM 1
#define INA228_DATA_NUM 1
#define PAC19XX_DATA_NUM 4

typedef enum {
    WR_ERROR,
    RD_ERROR_1,
    RD_ERROR_2,
    RD_ERROR_3,
    ARG_ERROR,
    NO_ERROR
} i2c_err_t;

typedef struct {
    uint8_t index;
    float tempHot[MCP960X_DATA_NUM];
    float tempCold[MCP960X_DATA_NUM];
} mcp960xData_t;

typedef struct {
    uint8_t index;
    float bus_volt[INA228_DATA_NUM];
    float shunt_volt[INA228_DATA_NUM];
} ina228Data_t;

typedef struct {
    uint8_t index;
    float bus_volt[PAC19XX_DATA_NUM];
    float shunt_volt[PAC19XX_DATA_NUM];
} pac19xxData_t;

#endif