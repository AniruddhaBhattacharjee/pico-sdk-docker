#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

#define MCP960X_DATA_NUM 3
#define INA228_DATA_NUM 1
#define PAC19XX_DATA_NUM 4

typedef enum {
    WR_ERROR,
    RD_ERROR_1,
    RD_ERROR_2,
    RD_ERROR_3,
    ARG_ERROR,
    NO_ERROR,
    OTHR_ERROR
} i2c_err_t;

typedef enum {
    START,
    INIT,
    MEASR,
    ER,
    STOP
} fsm_state_t;

typedef struct {
    fsm_state_t state;
} device_state_t;

#endif