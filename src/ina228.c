#include "ina228.h"
#include "ina228_reg.h"

static int32_t read_s24(uint8_t *buf, uint8_t resv_bits){
    int32_t val = (buf[0] << 16) | (buf[1] << 8) | buf[2];
    val = val >> resv_bits;
    // Sign extend
    if (val & 0x800000)
        val |= 0xFF000000;
    return val;
}

static uint16_t read_u16(uint8_t *buf, uint8_t resv_bits){
    return (buf[0] << 8) | buf[1];
}

i2c_err_t ina228_check_available(ina228_t *dev){
    uint8_t *rxdata;
    int ret = i2c_read_blocking(dev->i2c, dev->addr, rxdata, 1, false);
    if (ret < 0){
        return RD_ERROR_1;
    }
    else{
        return NO_ERROR;
    }
}

i2c_err_t ina228_init(ina228_t *dev){
    // Default config- continuous conversion
    uint16_t config = 0x0000;
    uint8_t buf[2] = { config >> 8, config & 0xFF };
    uint8_t reg = INA228_REG_CONFIG;
    int wret = i2c_write_blocking(dev->i2c, dev->addr, &reg, 1, false);
    //return i2c_write_reg(&dev->i2c, INA228_REG_CONFIG, buf, 2);
    if (wret < 0) return WR_ERROR;
    return NO_ERROR;
}

i2c_err_t ina228_set_calibration(ina228_t *dev, float max_current, float shunt_resistor){
    dev->shunt_resistor = shunt_resistor;

    // Choose current LSB
    dev->current_lsb = max_current / (1 << 19); // 20-bit ADC

    // datasheet Calibration register formula
    float cal = 13107.2f / (dev->current_lsb * shunt_resistor);

    uint16_t cal_reg = (uint16_t)cal;

    uint8_t buf[2] = {
        cal_reg >> 8,
        cal_reg & 0xFF
    };
    uint8_t reg = INA228_REG_SHUNT_CAL;
    int wret = i2c_write_blocking(dev->i2c, dev->addr, &reg, 1, false);
    //return i2c_write_reg(&dev->i2c, INA228_REG_SHUNT_CAL, buf, 2);
    if (wret < 0) return WR_ERROR;
    return NO_ERROR;
}

i2c_err_t ina228_read_bus_voltage(ina228_t *dev, float *voltage){
    uint8_t buf[3];
    // uint32_t buf;
    uint8_t reg = INA228_REG_BUS_VOLT;
    int wret = i2c_write_blocking(dev->i2c, dev->addr, &reg, 1, false);
    if (wret<0) return WR_ERROR;
    int rret = i2c_read_blocking(dev->i2c, dev->addr, buf, sizeof(buf), false);
    if (rret != (int)sizeof(buf)) return RD_ERROR_1;
    /*
    if (!i2c_read_reg(&dev->i2c, INA228_REG_BUS_VOLT, buf, 3))
        return false;
    */

    int32_t raw = read_s24(buf, 4);

    // LSB = 195.3125 uV
    *voltage = raw * 0.0001953125f;

    return NO_ERROR;
}

i2c_err_t ina228_read_shunt_voltage(ina228_t *dev, float *voltage){
    uint8_t buf[3];
    uint8_t reg = INA228_REG_SHUNT_VOLT;
    int wret = i2c_write_blocking(dev->i2c, dev->addr, &reg, 1, false);
    if (wret < 0) return WR_ERROR;
    int rret = i2c_read_blocking(dev->i2c, dev->addr, buf, sizeof(buf), false);
    if (rret != (int)sizeof(buf)) return RD_ERROR_1;
    /*
    if (!i2c_read_reg(&dev->i2c, INA228_REG_SHUNT_VOLT, buf, 3))
        return false;
    */
    int32_t raw = read_s24(buf, 4);

    // LSB = 312.5 nV
    *voltage = raw * 0.0000003125f;

    return NO_ERROR;
}

i2c_err_t ina228_read_current(ina228_t *dev, float *current){
    uint8_t buf[3];
    uint8_t reg = INA228_REG_CURRENT;
    int wret = i2c_write_blocking(dev->i2c, dev->addr, &reg, 1, false);
    if (wret < 0) return WR_ERROR;
    int rret = i2c_read_blocking(dev->i2c, dev->addr, buf, sizeof(buf), false);
    if (rret != (int)sizeof(buf)) return RD_ERROR_1;
    /*
    if (!i2c_read_reg(&dev->i2c, INA228_REG_CURRENT, buf, 3))
        return false;
    */
    int32_t raw = read_s24(buf, 4);

    *current = raw * dev->current_lsb;

    return NO_ERROR;
}

i2c_err_t ina228_read_power(ina228_t *dev, float *power){
    uint8_t buf[3];
    uint8_t reg = INA228_REG_POWER;
    int wret = i2c_write_blocking(dev->i2c, dev->addr, &reg, 1, false);
    if (wret < 0) return WR_ERROR;
    int rret = i2c_read_blocking(dev->i2c, dev->addr, buf, sizeof(buf), false);
    if (rret != (int)sizeof(buf)) return RD_ERROR_1;
    /*
    if (!i2c_read_reg(&dev->i2c, INA228_REG_POWER, buf, 3))
        return false;
    */
    int32_t raw = read_s24(buf, 0);

    // Power LSB = 3.2 * current_lsb
    *power = raw * (3.2f * dev->current_lsb);

    return NO_ERROR;
}

i2c_err_t ina228_read_energy(ina228_t *dev, double *energy){
    uint8_t buf[5];
    uint8_t reg = INA228_REG_ENERGY;
    int wret = i2c_write_blocking(dev->i2c, dev->addr, &reg, 1, false);
    if (wret < 0) return WR_ERROR;
    int rret = i2c_read_blocking(dev->i2c, dev->addr, buf, sizeof(buf), false);
    if (rret != (int)sizeof(buf)) return RD_ERROR_1;
    /*
    if (!i2c_read_reg(&dev->i2c, INA228_REG_ENERGY, buf, 5))
        return false;
    */
        uint64_t raw =
        ((uint64_t)buf[0] << 32) | ((uint64_t)buf[1] << 24) |
        ((uint64_t)buf[2] << 16) | ((uint64_t)buf[3] << 8)  |
        buf[4];

    // Energy LSB = 16 * power_lsb
    *energy = raw * (16.0 * 3.2 * dev->current_lsb);

    return NO_ERROR;
}
