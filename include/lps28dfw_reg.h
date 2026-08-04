#ifndef LPS28DFW_REG_H
#define LPS28DFW_REG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/******************************************************************************
 * Device Information
 ******************************************************************************/

#define LPS28DFW_WHO_AM_I_VALUE            0xB4

/******************************************************************************
 * I2C Addresses
 *
 * SDO = GND  -> 0x5C
 * SDO = VDD  -> 0x5D
 ******************************************************************************/

#define LPS28DFW_I2C_ADDR_LOW             0x5C
#define LPS28DFW_I2C_ADDR_HIGH            0x5D

/******************************************************************************
 * Register Map
 ******************************************************************************/

#define LPS28DFW_INTERRUPT_CFG            0x0B
#define LPS28DFW_THS_P_L                  0x0C
#define LPS28DFW_THS_P_H                  0x0D

#define LPS28DFW_IF_CTRL                  0x0E

#define LPS28DFW_WHO_AM_I                 0x0F

#define LPS28DFW_CTRL_REG1                0x10
#define LPS28DFW_CTRL_REG2                0x11
#define LPS28DFW_CTRL_REG3                0x12

#define LPS28DFW_CTRL_REG1_FS_MODE      (1 << 4)

#define LPS28DFW_FIFO_CTRL                0x14

#define LPS28DFW_REF_P_XL                 0x15
#define LPS28DFW_REF_P_L                  0x16
#define LPS28DFW_REF_P_H                  0x17

#define LPS28DFW_RPDS_L                   0x18
#define LPS28DFW_RPDS_H                   0x19

#define LPS28DFW_INT_SOURCE               0x24

#define LPS28DFW_FIFO_STATUS1             0x25
#define LPS28DFW_FIFO_STATUS2             0x26

#define LPS28DFW_STATUS                   0x27

#define LPS28DFW_PRESS_OUT_XL             0x28
#define LPS28DFW_PRESS_OUT_L              0x29
#define LPS28DFW_PRESS_OUT_H              0x2A

#define LPS28DFW_TEMP_OUT_L               0x2B
#define LPS28DFW_TEMP_OUT_H               0x2C

#define LPS28DFW_FIFO_DATA_OUT_PRESS_XL   0x78
#define LPS28DFW_FIFO_DATA_OUT_PRESS_L    0x79
#define LPS28DFW_FIFO_DATA_OUT_PRESS_H    0x7A

#define LPS28DFW_FIFO_DATA_OUT_TEMP_L     0x7B
#define LPS28DFW_FIFO_DATA_OUT_TEMP_H     0x7C

/******************************************************************************
 * STATUS Register Bits
 ******************************************************************************/

#define LPS28DFW_STATUS_P_DA             (1 << 0)
#define LPS28DFW_STATUS_T_DA             (1 << 1)
#define LPS28DFW_STATUS_P_OR             (1 << 4)
#define LPS28DFW_STATUS_T_OR             (1 << 5)

/******************************************************************************
 * CTRL_REG2 Bits
 ******************************************************************************/

#define LPS28DFW_SWRESET                 (1 << 2)
#define LPS28DFW_ONE_SHOT                (1 << 0)
#define LPS28DFW_IF_ADD_INC              (1 << 4)
#define LPS28DFW_BDU                     (1 << 3)
#define LPS28DFW_BOOT                    (1 << 7)

/******************************************************************************
 * CTRL_REG3 Bits
 ******************************************************************************/

#define LPS28DFW_LIR                     (1 << 2)
#define LPS28DFW_PP_OD                   (1 << 4)
#define LPS28DFW_INT_H_L                 (1 << 5)

/******************************************************************************
 * FIFO Modes
 ******************************************************************************/

typedef enum
{
    LPS28DFW_FIFO_BYPASS          = 0,
    LPS28DFW_FIFO_FIFO            = 1,
    LPS28DFW_FIFO_STREAM          = 2,
    LPS28DFW_FIFO_STREAM_TO_FIFO  = 3,
    LPS28DFW_FIFO_BYPASS_TO_STREAM= 4

} lps28dfw_fifo_mode_t;

/******************************************************************************
 * Output Data Rate
 ******************************************************************************/

typedef enum
{
    LPS28DFW_ODR_POWERDOWN = 0,

    LPS28DFW_ODR_1HZ,
    LPS28DFW_ODR_4HZ,
    LPS28DFW_ODR_10HZ,
    LPS28DFW_ODR_25HZ,
    LPS28DFW_ODR_50HZ,
    LPS28DFW_ODR_75HZ,
    LPS28DFW_ODR_100HZ,
    LPS28DFW_ODR_200HZ

} lps28dfw_odr_t;

/******************************************************************************
 * Low Pass Filter
 ******************************************************************************/

typedef enum
{
    LPS28DFW_LPF_DISABLE = 0,
    LPS28DFW_LPF_ODR_DIV4,
    LPS28DFW_LPF_ODR_DIV9,
    LPS28DFW_LPF_ODR_DIV20

} lps28dfw_lpf_t;

typedef enum
{
    LPS28DFW_FS_MODE_1260 = 0,
    LPS28DFW_FS_MODE_4060 = 1

} lps28dfw_fs_mode_t;

#ifdef __cplusplus
}
#endif

#endif