#ifndef PERIPH_DEFS_H
#define PERIPH_DEFS_H

#define I2C_PORT i2c0
#define I2C_SDA_PIN 16
#define I2C_SCL_PIN 17
#define I2C_BUS_FREQ (100*1000) // 100 KHz i2c speed

#define GPIO_TOGGLE_PIN 2
#define GPIO_TEST_PIN 3

#define UART1_TX_PIN 8
#define UART1_RX_PIN 9
#define UART_PORT uart1

#define INA228_SHUNT_RES (float)(0.015)
#define INA228_MAX_CURRENT (float)(10.0)

#endif