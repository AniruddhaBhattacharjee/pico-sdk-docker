#include "pac19xx.h"
#include "pac19xx_reg.h"

/* register maps */

static const uint8_t vbus_reg[4] ={
    PAC19XX_REG_VBUS1,
    PAC19XX_REG_VBUS2,
    PAC19XX_REG_VBUS3,
    PAC19XX_REG_VBUS4
};

static const uint8_t vsense_reg[4] ={
    PAC19XX_REG_VSENSE1,
    PAC19XX_REG_VSENSE2,
    PAC19XX_REG_VSENSE3,
    PAC19XX_REG_VSENSE4
};

static const uint8_t vpower_reg[4] ={
    PAC19XX_REG_VPOWER1,
    PAC19XX_REG_VPOWER2,
    PAC19XX_REG_VPOWER3,
    PAC19XX_REG_VPOWER4
};

static uint16_t read_u16(uint8_t *buf){
    return (buf[0] << 8) | buf[1];
}

static int16_t read_s16(uint8_t *buf){
    return (int16_t)((buf[0] << 8) | buf[1]);
}

static int32_t read_s32(uint8_t *buf){
    return (int32_t)(
        (buf[0] << 24) |
        (buf[1] << 16) |
        (buf[2] << 8)  |
        buf[3]);
}

bool pac19xx_init(pac19xx_t *dev){
    return true;
}

bool pac19xx_refresh(pac19xx_t *dev){
    uint8_t cmd = PAC19XX_CMD_REFRESH;
    return i2c_write_reg(&dev->i2c, cmd, NULL, 0);
}

bool pac19xx_read_bus_voltage(pac19xx_t *dev, int ch, float *voltage){
    if (ch < 0 || ch >= 4)
        return false;

    uint8_t buf[2];

    if (!i2c_read_reg(&dev->i2c, vbus_reg[ch], buf, 2))
        return false;

    uint16_t raw = read_u16(buf);

    /* 32V full scale */
    *voltage = (raw * 32.0f) / 65536.0f;

    return true;
}

bool pac19xx_read_sense_voltage(pac19xx_t *dev, int ch, float *voltage){
    if (ch < 0 || ch >= 4)
        return false;

    uint8_t buf[2];
    if (!i2c_read_reg(&dev->i2c, vsense_reg[ch], buf, 2))
        return false;

    int16_t raw = read_s16(buf);
    // ±100 mV full scale
    *voltage = (raw * 0.1f) / 32768.0f;
    return true;
}

bool pac19xx_read_current(pac19xx_t *dev, int ch, float *current){
    float vsense;
    if (!pac19xx_read_sense_voltage(dev, ch, &vsense))
        return false;

    float r = dev->shunt_resistor[ch];
    if (r <= 0)
        return false;

    *current = vsense / r;
    return true;
}

bool pac19xx_read_power(pac19xx_t *dev, int ch, float *power){
    float vbus;
    float current;
    if (!pac19xx_read_bus_voltage(dev, ch, &vbus))
        return false;

    if (!pac19xx_read_current(dev, ch, &current))
        return false;

    *power = vbus * current;
    return true;
}
