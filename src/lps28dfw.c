/******************************************************************************
 *  lps28dfw.c
 *  Driver for ST LPS28DFW Pressure Sensor
 ******************************************************************************/

#include "lps28dfw.h"

#include <string.h>

#ifndef LPS28DFW_RESET_TIMEOUT
#define LPS28DFW_RESET_TIMEOUT 100
#endif

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

i2c_err_t lps28dfw_read_reg(lps28dfw_t *dev,uint8_t reg,uint8_t *buf,size_t len){
    if (dev == NULL || buf == NULL)
        return RD_ERROR_1;

    int ret;
    ret = i2c_write_blocking(dev->i2c,dev->addr,&reg,1,true);
    if (ret != 1)
        return RD_ERROR_1;
    ret = i2c_read_blocking(dev->i2c,dev->addr,buf,len,false);
    if (ret != (int)len)
        return RD_ERROR_1;

    return NO_ERROR;
}

/******************************************************************************/

i2c_err_t lps28dfw_write_reg(lps28dfw_t *dev,uint8_t reg,uint8_t value){
    if (dev == NULL)
        return ARG_ERROR;

    uint8_t tx[2];
    tx[0] = reg;
    tx[1] = value;
    int ret = i2c_write_blocking(dev->i2c,dev->addr,tx,2,false);
    if (ret < 0) return WR_ERROR;

    return NO_ERROR;
}

/******************************************************************************/

i2c_err_t lps28dfw_read_register(lps28dfw_t *dev,uint8_t reg,uint8_t *data,size_t len){
    return lps28dfw_read_reg(dev,reg,data,len);
}

/******************************************************************************/

i2c_err_t lps28dfw_write_register(lps28dfw_t *dev,uint8_t reg,const uint8_t *data,size_t len){
    if (dev == NULL || data == NULL)
        return RD_ERROR_1;

    uint8_t tx[16];

    if (len > (sizeof(tx) - 1))
        return RD_ERROR_1;

    tx[0] = reg;
    memcpy(&tx[1], data, len);
    int ret = i2c_write_blocking(dev->i2c,dev->addr,tx,len + 1,false);
    if (ret != (int)(len + 1))
        return WR_ERROR;

    return NO_ERROR;
}

/******************************************************************************
 * Device Identification
 ******************************************************************************/

int lps28dfw_check_available(lps28dfw_t *dev){
    uint8_t whoami = 0;
    if (lps28dfw_read_reg(dev,LPS28DFW_WHO_AM_I,&whoami,1) != NO_ERROR)
    {
        return RD_ERROR_1;
    }

    if (whoami != LPS28DFW_WHO_AM_I_VALUE)
        return RD_ERROR_1;

    return NO_ERROR;
}

/******************************************************************************
 * Software Reset
 ******************************************************************************/

i2c_err_t lps28dfw_soft_reset(lps28dfw_t *dev){
    uint8_t ctrl2;
    if (lps28dfw_read_reg(dev,LPS28DFW_CTRL_REG2,&ctrl2,1) != NO_ERROR){
        return RD_ERROR_1;
    }

    ctrl2 |= LPS28DFW_SWRESET;

    if (lps28dfw_write_reg(dev,LPS28DFW_CTRL_REG2,ctrl2) != NO_ERROR){
        return RD_ERROR_1;
    }

    uint32_t timeout = 0;

    while (timeout++ < LPS28DFW_RESET_TIMEOUT){
        if (lps28dfw_read_reg(dev,LPS28DFW_CTRL_REG2,&ctrl2,1) != NO_ERROR){
            return RD_ERROR_1;
        }
        if (!(ctrl2 & LPS28DFW_SWRESET))
            return NO_ERROR;
        sleep_ms(1);
    }

    return NO_ERROR;
}

/******************************************************************************
 * Setting Output Data Rate
 ******************************************************************************/

i2c_err_t lps28dfw_set_odr(lps28dfw_t *dev, lps28dfw_odr_t odr){
    uint8_t ctrl1;
    if (lps28dfw_read_reg(dev, LPS28DFW_CTRL_REG1, &ctrl1, 1) != NO_ERROR){
        return RD_ERROR_1;
    }
    ctrl1 &= ~(0x0F << 3);
    ctrl1 |= ((uint8_t)odr << 3);
    return lps28dfw_write_reg(dev, LPS28DFW_CTRL_REG1, ctrl1);
}

/******************************************************************************
 * Low Pass Filter
 ******************************************************************************/

i2c_err_t lps28dfw_enable_lowpass(lps28dfw_t *dev, lps28dfw_lpf_t mode){
    uint8_t ctrl1;
    if (lps28dfw_read_reg(dev, LPS28DFW_CTRL_REG1, &ctrl1, 1) != NO_ERROR){
        return RD_ERROR_1;
    }

    ctrl1 &= ~(0x03);
    ctrl1 |= (uint8_t)mode;
    return lps28dfw_write_reg(dev, LPS28DFW_CTRL_REG1, ctrl1);
}

/******************************************************************************
 * Configuration
 ******************************************************************************/

i2c_err_t lps28dfw_set_config(lps28dfw_t *dev, const lps28dfw_config_t *cfg){
    if (cfg == NULL)
        return ARG_ERROR;

    uint8_t ctrl2 = 0;
    if (cfg->enable_bdu)
        ctrl2 |= LPS28DFW_BDU;

    if (cfg->auto_increment)
        ctrl2 |= LPS28DFW_IF_ADD_INC;

    if (lps28dfw_write_reg(dev, LPS28DFW_CTRL_REG2, ctrl2) != NO_ERROR){
        return WR_ERROR;
    }

    if (lps28dfw_set_odr(dev, cfg->odr) != NO_ERROR){
        return OTHR_ERROR;
    }

    if (lps28dfw_enable_lowpass(dev, cfg->lpf) != NO_ERROR){
        return OTHR_ERROR;
    }

    if (lps28dfw_set_fs_mode(dev, cfg->fs_mode)!=NO_ERROR){
        return WR_ERROR;
    }
    return NO_ERROR;
}

/******************************************************************************
 * Initialization
 ******************************************************************************/

i2c_err_t lps28dfw_init(lps28dfw_t *dev){
    if (lps28dfw_check_available(dev) != NO_ERROR)
        return OTHR_ERROR;
    if (lps28dfw_soft_reset(dev) != NO_ERROR)
        return OTHR_ERROR;
    
        lps28dfw_config_t cfg ={
        .odr = LPS28DFW_ODR_25HZ,
        .lpf = LPS28DFW_LPF_ODR_DIV4,
        .enable_bdu = true,
        .auto_increment = true,
        .fs_mode = 0
    };

    return lps28dfw_set_config(dev, &cfg);
}

/******************************************************************************
 * Raw Pressure Read
 ******************************************************************************/

i2c_err_t lps28dfw_read_pressure_raw(lps28dfw_t *dev,int32_t *raw_pressure){
    uint8_t buf[3];
    if ((dev == NULL) || (raw_pressure == NULL))
        return ARG_ERROR;

    if (lps28dfw_read_reg(dev,LPS28DFW_PRESS_OUT_XL,buf,sizeof(buf)) != NO_ERROR){
        return RD_ERROR_1;
    }

    int32_t raw = ((int32_t)buf[2] << 16) | ((int32_t)buf[1] << 8) | ((int32_t)buf[0]);
    // Sign extend 24-bit number
    if (raw & 0x00800000)
        raw |= 0xFF000000;

    *raw_pressure = raw;
    return NO_ERROR;
}

/******************************************************************************
 * Raw Temperature Read
 ******************************************************************************/

i2c_err_t lps28dfw_read_temperature_raw(lps28dfw_t *dev,int16_t *raw_temperature){
    uint8_t buf[2];

    if ((dev == NULL) || (raw_temperature == NULL))
        return ARG_ERROR;

    if (lps28dfw_read_reg(dev,LPS28DFW_TEMP_OUT_L,buf,sizeof(buf)) != NO_ERROR){
        return RD_ERROR_1;
    }

    *raw_temperature = (int16_t)(((uint16_t)buf[1] << 8) | (uint16_t)buf[0]);

    return NO_ERROR;
}

/******************************************************************************
 * Burst Read Pressure + Temperature
 ******************************************************************************/

i2c_err_t lps28dfw_read_raw(lps28dfw_t *dev, int32_t *raw_pressure, int16_t *raw_temperature){
    uint8_t buf[5];

    if ((dev == NULL) || (raw_pressure == NULL) || (raw_temperature == NULL)){
        return ARG_ERROR;
    }

    if (lps28dfw_read_reg(dev,LPS28DFW_PRESS_OUT_XL,buf,sizeof(buf)) != NO_ERROR){
        return RD_ERROR_1;
    }

    int32_t pressure = ((int32_t)buf[2] << 16) | ((int32_t)buf[1] << 8) | ((int32_t)buf[0]);
    if (pressure & 0x00800000)
        pressure |= 0xFF000000;

    *raw_pressure = pressure;
    *raw_temperature = (int16_t)(((uint16_t)buf[4] << 8) | (uint16_t)buf[3]);
    return NO_ERROR;
}

/******************************************************************************
 * Conversion Functions
 ******************************************************************************/

float lps28dfw_convert_pressure(int32_t raw_pressure, lps28dfw_fs_mode_t mode){
    if (mode == LPS28DFW_FS_MODE_1260){
        return ((float)raw_pressure) / 4096.0f;
    } 
    else if (mode == LPS28DFW_FS_MODE_4060){
        return ((float)raw_pressure) / 2048.0f;
    }
    //return ((float)raw_pressure) / 4096.0f;
}

/******************************************************************************/

float lps28dfw_convert_temperature(int16_t raw_temperature){
    return ((float)raw_temperature) / 100.0f;
}

/******************************************************************************
 * Floating Point Pressure
 ******************************************************************************/

i2c_err_t lps28dfw_read_pressure(lps28dfw_t *dev,float *pressure_hpa, lps28dfw_fs_mode_t mode){
    int32_t raw;
    if (pressure_hpa == NULL)
        return ARG_ERROR;

    if (lps28dfw_read_pressure_raw(dev,&raw) != NO_ERROR){
        return OTHR_ERROR;
    }
    *pressure_hpa = lps28dfw_convert_pressure(raw, mode);
    return NO_ERROR;
}

/******************************************************************************
 * Floating Point Temperature
 ******************************************************************************/

i2c_err_t lps28dfw_read_temperature(lps28dfw_t *dev,float *temperature_c){
    int16_t raw;
    if (temperature_c == NULL)
        return ARG_ERROR;

    if (lps28dfw_read_temperature_raw(dev,&raw) != NO_ERROR){
        return RD_ERROR_2;
    }

    *temperature_c = lps28dfw_convert_temperature(raw);
    return NO_ERROR;
}

/******************************************************************************
 * Read Both Pressure + Temperature
 ******************************************************************************/

i2c_err_t lps28dfw_read_all(lps28dfw_t *dev,float *pressure_hpa,float *temperature_c, lps28dfw_fs_mode_t mode){
    int32_t rawPressure;
    int16_t rawTemperature;

    if ((pressure_hpa == NULL) ||(temperature_c == NULL)){
        return ARG_ERROR;
    }

    if (lps28dfw_read_raw(dev,&rawPressure,&rawTemperature) != NO_ERROR){
        return RD_ERROR_2;
    }
    *pressure_hpa = lps28dfw_convert_pressure(rawPressure, mode);
    *temperature_c = lps28dfw_convert_temperature(rawTemperature);
    return NO_ERROR;
}

/******************************************************************************
 * Status Register
 ******************************************************************************/

i2c_err_t lps28dfw_get_status(lps28dfw_t *dev,uint8_t *status){
    return lps28dfw_read_reg(dev,LPS28DFW_STATUS,status,1);
}

/******************************************************************************/

bool lps28dfw_pressure_ready(lps28dfw_t *dev){
    uint8_t status;
    if (lps28dfw_get_status(dev, &status) != NO_ERROR)
        return false;

    return (status & LPS28DFW_STATUS_P_DA);
}

/******************************************************************************/

bool lps28dfw_temperature_ready(lps28dfw_t *dev){
    uint8_t status;
    if (lps28dfw_get_status(dev, &status) != NO_ERROR)
        return false;

    return (status & LPS28DFW_STATUS_T_DA);
}

/******************************************************************************
 * FIFO
 ******************************************************************************/

i2c_err_t lps28dfw_fifo_set_mode(lps28dfw_t *dev, lps28dfw_fifo_mode_t mode){
    uint8_t reg = (uint8_t)mode;
    return lps28dfw_write_reg(dev,LPS28DFW_FIFO_CTRL,reg);
}

/******************************************************************************/

int lps28dfw_fifo_get_level(lps28dfw_t *dev,uint8_t *level){
    uint8_t status;
    if ((dev == NULL) || (level == NULL))
        return ARG_ERROR;
    if (lps28dfw_read_reg(dev, LPS28DFW_FIFO_STATUS1, &status, 1) != NO_ERROR){
        return RD_ERROR_2;
    }
    *level = status;
    return NO_ERROR;
}

/******************************************************************************/

int lps28dfw_fifo_read_sample(lps28dfw_t *dev,float *pressure, float *temperature, lps28dfw_fs_mode_t mode){
    return lps28dfw_read_all(dev,pressure,temperature, mode);
}

i2c_err_t lps28dfw_set_fs_mode(lps28dfw_t *dev, lps28dfw_fs_mode_t mode){
    uint8_t ctrl1;
    if (dev == NULL)
        return ARG_ERROR;

    if (lps28dfw_read_reg(dev, LPS28DFW_CTRL_REG1, &ctrl1, 1) != NO_ERROR){
        return RD_ERROR_1;
    }

    // Clear FS_MODE bit
    ctrl1 &= ~LPS28DFW_CTRL_REG1_FS_MODE;

    // Set requested mode
    if (mode == LPS28DFW_FS_MODE_4060){
        ctrl1 |= LPS28DFW_CTRL_REG1_FS_MODE;
    }

    return lps28dfw_write_reg(dev, LPS28DFW_CTRL_REG1, ctrl1);
}