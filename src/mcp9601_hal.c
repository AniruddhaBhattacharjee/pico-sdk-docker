#include "mcp9601_reg.h"
#include "mcp9601_hal.h"
#include <stdint.h>
/**
 * Checks whether specific MCP9601 i2c device is present on i2c bus
 * 
 * @param i2c Instance of I2C port, type: "i2c_inst_t*"
 * @param addr Address of MCP9601 device
 * @param rxdata Data buffer to hold byte read from i2c device
 * @return NUmber of bytes read when device read is succeessful, otherwise 0. 
 */
uint8_t mcp9601_check_available(i2c_inst_t* i2c, uint8_t addr, uint8_t* rxdata){
    int ret = i2c_read_blocking(i2c, addr, rxdata, 1, false);
    if (ret < 0){
        return 0;
    }
    else{
        return ret;
    }
}

/**
 * Scans the I2C bus for MCP9601 devices (address b/w 0x60 - 0x67)
 * 
 * @param i2c Instance of the i2c port, type: "i2c_inst_t*"
 * @return Number of MCP9601 devices found
 */
uint8_t mcp9601_device_scan(i2c_inst_t* i2c){
    uint8_t *rxdata;
    int ret, i, cnt=1;
    for (i = MCP9601_MIN_ADDR; i <= MCP9601_MAX_ADDR; i++){
        ret = i2c_read_blocking(i2c, i, rxdata, 1, false);
        if (ret >= 0){
            cnt++;
        }
    }
    return cnt;
}

uint8_t mcp9601_set_device_config(mcp9601_t *dev, tcold_res_t tcoldres, adc_res_t adcres){
    // form the device config register settings byte
    uint8_t reg_value = 0x00;
    reg_value |= (tcoldres << MCP9601_TC_RES_SET_BIT) | (adcres << MCP9601_ADC_RES_SET_BIT);
    // write this to register
    uint8_t wreg = MCP9601_DEV_CONFIG_REG;
    uint8_t buf[2];
    buf[0] = wreg;
    buf[1] = reg_value;
    int wret = i2c_write_blocking(dev->i2c, dev->addr, buf, sizeof(buf), false);
    return (wret >= 0);
}

/**
 * Function to read Cold Temperature register from MCP9601
 * 
 * @param i2c Instance of the i2c port, type: "i2c_inst_t*"
 * @param addr Address of MCP9601 device
 * @param buf Pointer to array or variable capable of holding 2 bytes (uint8_t[2], uint16_t, etc.)
 * @return Number of bytes read if read is successful, otherwise Number 0.
 */
i2c_err_t mcp9601_read_tCold_reg(mcp9601_t *dev, uint8_t* buf, size_t buflen){
    // write the command to the device....
    if (sizeof(buf) != 2 && buflen != 2){
        return ARG_ERROR; 
    }
    uint8_t reg = MCP9601_TC_TEMP_REG;
    int wret = i2c_write_blocking(dev->i2c, dev->addr, &reg, 1, false);
    if(wret < 0) return WR_ERROR;

    int rret = i2c_read_blocking(dev->i2c, dev->addr, buf, buflen, false);
    return (rret == (int)buflen ? NO_ERROR : RD_ERROR_1);
}

/**
 * Function to read Hot Temperature register from MCP9601
 * 
 * @param i2c Instance of the i2c port, type: "i2c_inst_t*"
 * @param addr Address of MCP9601 device
 * @param buf Pointer to array or variable capable of holding 2 bytes (uint8_t[2], uint16_t, etc.)
 * @return Number of bytes read if read is successful, otherwise 0.
 */
i2c_err_t mcp9601_read_tHot_reg(mcp9601_t *dev, uint8_t* buf, size_t buflen){
    if (sizeof(buf) != 2 && buflen != 2){
        return ARG_ERROR; 
    }
    uint8_t reg = MCP9601_TH_TEMP_REG;
    int wret = i2c_write_blocking(dev->i2c, dev->addr, &reg, 1, false);
    if(wret < 0) return WR_ERROR;
    
    //if (wr_sleep_ms) sleep_ms(wr_sleep_ms);

    // Now, read data from the sensor
    int rret = i2c_read_blocking(dev->i2c, dev->addr, buf, buflen, false);
    return (rret == (int)buflen ? NO_ERROR : RD_ERROR_1);
}

/**
 * Function to read Delta Temperature register from MCP9601
 * 
 * @param i2c Instance of the i2c port, type: "i2c_inst_t*"
 * @param addr Address of MCP9601 device
 * @param buf Pointer to array or variable capable of holding 2 bytes (uint8_t[2], uint16_t, etc.)
 * @return Number of bytes read if read is successful, otherwise 0.
 */
i2c_err_t mcp9601_read_tDelta_reg(mcp9601_t *dev, uint8_t* buf, size_t buflen){
    if (sizeof(buf) != 2 && buflen != 2){
        return ARG_ERROR; 
    }
    uint8_t reg = MCP9601_TD_TEMP_REG;
    int wret = i2c_write_blocking(dev->i2c, dev->addr, &reg, 1, false);
    if(wret < 0) return WR_ERROR;
    
    //if (wr_sleep_ms) sleep_ms(wr_sleep_ms);

    // Now, read data from the sensor
    int rret = i2c_read_blocking(dev->i2c, dev->addr, buf, buflen, false);
    return (rret == (int)buflen ? NO_ERROR : RD_ERROR_1);
}

/**
 * Function to read Hot, Delta and Cold temperature registers sequentially.
 * 
 * @param i2c Instance of the i2c port, type: "i2c_inst_t*"
 * @param addr Address of MCP9601 device
 * @param buf Pointer to array or variable capable of holding 6 bytes (uint8_t[6], etc.).
 * @param buflen size of buf param
 * @return NO_ERROR (mcp_i2_err_t type) if successful, otherwise an X_ERROR_N value.  
 */
i2c_err_t mcp9601_read_alltemp_reg(mcp9601_t *dev, uint8_t* buf, size_t buflen){
    if (sizeof(buf) != 6 && buflen != 6){
        return ARG_ERROR; 
    }
    uint8_t reg = MCP9601_TH_TEMP_REG;
    int wret = i2c_write_blocking(dev->i2c, dev->addr, &reg, 1, false);
    if(wret < 0) return WR_ERROR;
    int rret = i2c_read_blocking(dev->i2c, dev->addr, buf, buflen, false);
    return (rret == (int)buflen ? NO_ERROR : RD_ERROR_1);
}


/**
 * Function to convert register value to temperature in Celsius.
 * 
 * @param buf Pointer to 2 bytes of data (uint8_t [2], uint16_t, etc.)
 * @return Temperature (in floating point).
 */
float mcp9601_convert_to_temp(uint8_t *buf){
    uint8_t upperbyte = buf[0];
    uint8_t lowerbyte = buf[1];
    float temp;
    if ((upperbyte & 0x80) == 0x80){
        // if sign bit shows < 0degC value
        temp = ((float)((float)upperbyte * (float)16.0) + (float)((float)lowerbyte / (float)16.0)) - (float)4096.0;
    }
    else{
        // if sign bit shows value > 0degC
        temp = ( ((float)upperbyte * (float)16.0) + ((float)lowerbyte / (float)16.0) );
    }
    return temp;
}


