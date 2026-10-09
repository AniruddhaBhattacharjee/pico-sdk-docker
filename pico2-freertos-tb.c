#include <stdio.h>
#include "pico/stdlib.h"
//#include "pico/time.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"

#include "common_defs.h"
#include "periph_defs.h"
#include "mcp9601_hal.h"
#include "ina228.h"
#include "pac19xx.h"
#include "uart_helpers.h"
#include "lps28dfw.h"
#include "disp_seg_driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define SAMPLE_PERIOD 8000 // in usec

/*
int64_t alarm_callback(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    return 0;
}
*/
// Freertos Task Handles
static TaskHandle_t sampling_taskHandle;
static TaskHandle_t mcp_rtemp_all_taskHandle;
static TaskHandle_t mcp_rtemp_hc_taskHandle;
static TaskHandle_t ina_vread_taskHandle;
static TaskHandle_t pac_vread_taskHandle;
static TaskHandle_t uart_transmit_taskHandle;
static TaskHandle_t lps28_read_taskHandle;
static TaskHandle_t dispSeg_taskHandle;
static TaskHandle_t blink_dp_taskHandle;

// Freertos Semaphore
static SemaphoreHandle_t pacState_mutex;
static SemaphoreHandle_t mcpState_mutex;
static SemaphoreHandle_t inaState_mutex;
static SemaphoreHandle_t lpsState_mutex;
static SemaphoreHandle_t devState_mutex;
static SemaphoreHandle_t i2cBus_mutex;
// declare sensor data objects
mcp960xData_t mcp9601Data;
pac19xxData_t pac1954Data;
ina228Data_t ina228Data;
lps28dfwData_t lps28Data;

// declare sensor device structs
/*
mcp9601_t mcp_dev1 = {
    .i2c = I2C_PORT,
    .addr = 0x67,
    .adc_res = RES_18B,
    .cold_res = HIGH_RES,
    .data_index = 0
};

mcp9601_t mcp_dev2 = {
    .i2c = I2C_PORT,
    .addr = 0x66,
    .adc_res = RES_18B,
    .cold_res = HIGH_RES,
    .data_index = 1
};

mcp9601_t mcp_dev3 = {
    .i2c = I2C_PORT,
    .addr = 0x65,
    .adc_res = RES_18B,
    .cold_res = HIGH_RES,
    .data_index = 2,
};
*/

mcp9601_t mcp_arr[MCP960X_DATA_NUM] = {
    {I2C_PORT, 0x65, RES_18B, HIGH_RES, 0},
    {I2C_PORT, 0x66, RES_18B, HIGH_RES, 1},
    {I2C_PORT, 0x67, RES_18B, HIGH_RES, 2}
};

// pac19xx_t pac19_dev1;
// ina228_t ina_dev1;

lps28dfw_t lps_dev1 = {
    .i2c = I2C_PORT,
    .addr = LPS28DFW_I2C_ADDR_LOW,
    .data_index = 0
};

lps28dfw_t lps_dev2 = {
    .i2c = I2C_PORT,
    .addr = LPS28DFW_I2C_ADDR_HIGH,
    .data_index = 1
};

lps28dfw_config_t cfg1 = {
    .odr            = LPS28DFW_ODR_25HZ,
    .lpf            = LPS28DFW_LPF_ODR_DIV4,
    .enable_bdu     = true,
    .auto_increment = true,
    .fs_mode = LPS28DFW_FS_MODE_1260
};

lps28dfw_config_t cfg2 = {
    .odr            = LPS28DFW_ODR_25HZ,
    .lpf            = LPS28DFW_LPF_ODR_DIV4,
    .enable_bdu     = true,
    .auto_increment = true,
    .fs_mode = LPS28DFW_FS_MODE_1260
};

device_state_t cur_state = {START};
device_state_t prev_state = {START};

// setup functions 
void initializeData() {
    cur_state.state = INIT;
    int i;
    mcp9601Data.index = 0;
    for (i = 0; i < MCP960X_DATA_NUM; ++i){
        mcp9601Data.tempCold[i] = 0.0f;
        mcp9601Data.tempHot[i] = 0.0f;
    }

    for (i = 0; i < INA228_DATA_NUM; ++i){
        ina228Data.bus_volt[i] = 0.0f;
        ina228Data.shunt_volt[i] = 0.0f;
    }
    
    for (i = 0; i < PAC19XX_DATA_NUM; ++i){
        pac1954Data.bus_volt[i] = 0.0f;
        pac1954Data.shunt_volt[i] = 0.0f;
    }

    for (i = 0; i < LPS28DFW_DATA_NUM; ++i){
        lps28Data.pressure[i] = 0.0f;
        lps28Data.temperature[i] = 0.0f;
    }
    return;
}

// Freertos task functions
void fsamplingTask(void *arg){
    xSemaphoreTake(devState_mutex, portMAX_DELAY);
    cur_state.state = (fsm_state_t)(MEASR);
    xSemaphoreGive(devState_mutex);

    uint32_t notify;
    for(;;) {
        xTaskNotifyWait(0, UINT32_MAX, &notify, portMAX_DELAY);
        xTaskNotify(mcp_rtemp_all_taskHandle, 0x01, eSetBits);
        // xTaskNotify(pac_vread_taskHandle, 0x01, eSetBits);
        // xTaskNotify(ina_vread_taskHandle, 0x01, eSetBits);
        xTaskNotify(lps28_read_taskHandle, 0x01, eSetBits);
        vTaskDelay(pdMS_TO_TICKS(1));
        xTaskNotify(uart_transmit_taskHandle, 0x01, eSetBits);
        xSemaphoreTake(devState_mutex, portMAX_DELAY);
        if (cur_state.state != prev_state.state) {
            xTaskNotify(dispSeg_taskHandle, 0x01, eSetBits);
            prev_state = cur_state;
        }
        xSemaphoreGive(devState_mutex);
    }
}

void fblink_dp_task(void *arg) {
    uint32_t notify;
    uint8_t pass;
    for(;;) {
        //xTaskNotifyWait(0, UINT32_MAX, &notify, portMAX_DELAY);
        
        xSemaphoreTake(devState_mutex, portMAX_DELAY);
        pass = cur_state.state != (fsm_state_t)(MEASR) ? 0 : 1;
        xSemaphoreGive(devState_mutex);

        if (!pass) {
            continue;
        } else {
            gpio_xor_mask(SEG_DP_MASK);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

void fdisp_seg_task(void *arg) {
    fsm_state_t dState;
    uint32_t notify;
    for (;;) {
        xTaskNotifyWait(0, UINT32_MAX, &notify, portMAX_DELAY);

        xSemaphoreTake(devState_mutex, portMAX_DELAY);
        dState = cur_state.state;
        xSemaphoreGive(devState_mutex);
        switch(dState) {
            case (fsm_state_t)(INIT):
                display_state_init();
                break;
            case (fsm_state_t)(START):
                display_state_start();
                break;
            case (fsm_state_t)(MEASR):
                display_state_measure();
                break;
            case (fsm_state_t)(ER):
                display_state_error();
                break;
            default:
                display_state_all();
        }
    }
}

void fmcp_rtemp_all_task(void *arg) {
    uint8_t temp_buf[6], tHot_buf[2], tCold_buf[2], tDelta_buf[2];
    float THot = 0.0f, TCold = 0.0f, TDelta = 0.0f;
    //uint8_t pin_state = 0;
    uint32_t notify;
    //uint32_t cur_t;
    for (;;) {
        xTaskNotifyWait(0, UINT32_MAX, &notify, portMAX_DELAY);
        xSemaphoreTake(i2cBus_mutex, portMAX_DELAY);
        for (int i = 0; i < MCP960X_DATA_NUM; ++i) {
            if (mcp9601_read_alltemp_reg((mcp_arr+i), temp_buf, sizeof(temp_buf)) == NO_ERROR) {
                tHot_buf[0] = temp_buf[0]; tHot_buf[1] = temp_buf[1];
                tCold_buf[0] = temp_buf[4]; tCold_buf[1] = temp_buf[5];
                //tDelta_buf[0] = temp_buf[2]; tDelta_buf[1] = temp_buf[3];
                THot = mcp9601_convert_to_temp(tHot_buf);
                TCold = mcp9601_convert_to_temp(tCold_buf);
            } else {
                THot = -1.0f; TCold = -1.0f;
            }
            xSemaphoreTake(mcpState_mutex, portMAX_DELAY);
            mcp9601Data.tempHot[i] = THot;
            mcp9601Data.tempCold[i] = TCold;
            xSemaphoreGive(mcpState_mutex);    
        }
        xSemaphoreGive(i2cBus_mutex);
        // pin_state = ~(pin_state);
        // gpio_put(GPIO_TOGGLE_PIN, pin_state);
        /*
        if(mcp9601_read_alltemp_reg(&mcp_dev1, temp_buf, sizeof(temp_buf)) == NO_ERROR){
            tHot_buf[0] = temp_buf[0]; tHot_buf[1] = temp_buf[1];
            tCold_buf[0] = temp_buf[4]; tCold_buf[1] = temp_buf[5];
            //tDelta_buf[0] = temp_buf[2]; tDelta_buf[1] = temp_buf[3];
            THot = mcp9601_convert_to_temp(tHot_buf);
            TCold = mcp9601_convert_to_temp(tCold_buf);
            //TDelta = mcp9601_convert_to_temp(tDelta_buf);
        }else{
            //uart_puts(UART_PORT, "ALL TEMP READ ERROR!\n");
            THot = -1.0f; TCold = -1.0f;
        }
        xSemaphoreTake(mcpState_mutex, portMAX_DELAY);
        mcp9601Data.tempHot[0] = THot;
        mcp9601Data.tempCold[0] = TCold;
        //mcp9601Data.index = (mcp9601Data.index + 1) % MCP960X_DATA_NUM;
        xSemaphoreGive(mcpState_mutex);
    }
}


int64_t alarm_callback(alarm_id_t id, void *user_data) {
    BaseType_t xHigherPriorityTaskWoken
        //uart_fprint(UART_PORT, THot, 3, ',');
        //uart_fprint(UART_PORT, TCold, 3, '\n');
        */
    }
}

void flps28ReadTask(void *arg){
    uint32_t notify;
    float pressure1, temperature1, pressure2, temperature2;
    for (;;){
        xTaskNotifyWait(0, UINT32_MAX, &notify, portMAX_DELAY);
        xSemaphoreTake(i2cBus_mutex, portMAX_DELAY);
        if (lps28dfw_read_all(&lps_dev1, &pressure1, &temperature1, cfg1.fs_mode) == NO_ERROR && lps28dfw_read_all(&lps_dev2, &pressure2, &temperature2, cfg2.fs_mode) == NO_ERROR){
            xSemaphoreTake(lpsState_mutex, portMAX_DELAY);
            lps28Data.pressure[lps_dev1.data_index] = pressure1;
            lps28Data.temperature[lps_dev1.data_index] = temperature1;
            lps28Data.pressure[lps_dev2.data_index] = pressure2;
            lps28Data.temperature[lps_dev2.data_index] = temperature2;
            xSemaphoreGive(lpsState_mutex);
        }
        else{
            xSemaphoreTake(lpsState_mutex, portMAX_DELAY);
            lps28Data.pressure[lps_dev1.data_index] = 0.0f;
            lps28Data.temperature[lps_dev1.data_index] = 0.0f;
            lps28Data.pressure[lps_dev2.data_index] = 0.0f;
            lps28Data.temperature[lps_dev2.data_index] = 0.0f;
            xSemaphoreGive(lpsState_mutex);
        }
        /*
        if (lps28dfw_read_all(&lps_dev2, &pressure, &temperature, cfg2.fs_mode) == NO_ERROR){
            xSemaphoreTake(lpsState_mutex, portMAX_DELAY);
            lps28Data.pressure[lps_dev2.data_index] = pressure;
            lps28Data.temperature[lps_dev2.data_index] = temperature;
            xSemaphoreGive(lpsState_mutex);
        }
        else{
            xSemaphoreTake(lpsState_mutex, portMAX_DELAY);
            lps28Data.pressure[lps_dev2.data_index] = 0.0f;
            lps28Data.temperature[lps_dev2.data_index] = 0.0f;
            xSemaphoreGive(lpsState_mutex);
        }

        */
        xSemaphoreGive(i2cBus_mutex);
    }
}

/*
void fmcp_rtemp_hc_task(void *arg){
    uint8_t tHot_buf[2], tCold_buf[2];
    float THot = 0.0f, TCold = 0.0f;
    uint32_t notify;

    for(;;){
        xTaskNotifyWait(0, UINT32_MAX, &notify, portMAX_DELAY);
        if (mcp9601_read_tHot_reg(&mcp_dev1, tHot_buf, sizeof(tHot_buf)) == NO_ERROR){
            THot = mcp9601_convert_to_temp(tHot_buf);
        }
        else{
            //uart_puts(UART_PORT, "Hot Temp read Error!\n");
            THot = -1.0f;
        }
        if (mcp9601_read_tCold_reg(&mcp_dev1, tCold_buf, sizeof(tCold_buf)) == NO_ERROR){
            TCold = mcp9601_convert_to_temp(tCold_buf);
        }
        else{
            //uart_puts(UART_PORT, "Cold Temp read Error!\n");
            TCold = -1.0f;
        }
        //uart_fprint(UART_PORT, THot, 3, ',');
        //uart_fprint(UART_PORT, TCold, 3, '\n');
        xSemaphoreTake(mcpState_mutex, portMAX_DELAY);
        mcp9601Data.tempHot[0] = THot;
        mcp9601Data.tempCold[0] = TCold;
        //mcp9601Data.index = (mcp9601Data.index + 1) % MCP960X_DATA_NUM;
        xSemaphoreGive(mcpState_mutex);

    }
}
*/

/*
void fpac19VoltReadTask(void *arg) {
    uint32_t notify;
    uint8_t channel = 0;
    float busVolt = 0.0f, shuntVolt = 0.0f;
    for(;;){
        xTaskNotifyWait(0, UINT32_MAX, &notify, portMAX_DELAY);
        if (pac19xx_refresh(&pac19_dev1) != NO_ERROR){
            busVolt = -1.0f; shuntVolt = -1.0f;
        }
        if (pac19xx_read_bus_voltage(&pac19_dev1, channel, &busVolt) != NO_ERROR){
            busVolt = -1.0f;
        }
        if (pac19xx_read_shunt_voltage(&pac19_dev1, channel, &shuntVolt) != NO_ERROR){
            shuntVolt = -1.0f;
        }
        xSemaphoreTake(pacState_mutex, portMAX_DELAY);
        pac1954Data.shunt_volt[0] = shuntVolt;
        pac1954Data.bus_volt[0] = busVolt;
        xSemaphoreGive(pacState_mutex);
    }
}
*/

/*
void finaVoltReadTask(void *arg) {
    uint32_t notify;
    uint8_t channel;
    float busVolt = 0.0f, shuntVolt = 0.0f;
    for(;;){
        xTaskNotifyWait(0, UINT32_MAX, &notify, portMAX_DELAY);
        if (ina228_read_bus_voltage(&ina_dev1, &busVolt) != NO_ERROR){
            busVolt = -1.0f;
        }
        if (ina228_read_shunt_voltage(&ina_dev1, &shuntVolt) != NO_ERROR){
            shuntVolt = -1.0f;
        }
        xSemaphoreTake(inaState_mutex, portMAX_DELAY);
        ina228Data.shunt_volt[0] = shuntVolt;
        ina228Data.bus_volt[0] = busVolt;
        xSemaphoreGive(inaState_mutex);
    }
}
*/
/*
void fuartTransmitTask(void *arg) {
    uint32_t notify;
    int i = 0;
    char f[(LPS28DFW_DATA_NUM)+(MCP960X_DATA_NUM)][16], line[160];
    for (;;) {
        xTaskNotifyWait(0, UINT32_MAX, &notify, portMAX_DELAY);
        xSemaphoreTake(lpsState_mutex, portMAX_DELAY);
        while (i < LPS28DFW_DATA_NUM) {
            float_to_string(lps28Data.pressure[i], f[i], 2, '\0');
            float_to_string(lps28Data.temperature[i], f[i+1], 2, '\0');
            i = i + 1;
        }
        xSemaphoreGive(lpsState_mutex);
        xSemaphoreTake(mcpState_mutex, portMAX_DELAY);
        for (i = 0; i < MCP960X_DATA_NUM; ++i) {
            float_to_string(mcp9601Data.tempHot[i], f[(2*LPS28DFW_DATA_NUM ) + i], 2, '\0');
            float_to_string(mcp9601Data.tempCold[i], f[(2*LPS28DFW_DATA_NUM) + i + 1], 2, '\0');
        }
        xSemaphoreGive(mcpState_mutex);
        snprintf(line, sizeof(line), "%.0s%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n", "", f[0],f[1],f[2],f[3],f[4],f[5],f[6],f[7],f[8],f[9]);
        uart_puts(UART_PORT, line);
    }
}
*/

void fuartTransmitTask(void *arg) {
    uint32_t notify;
    for (;;) {
        xTaskNotifyWait(0, UINT32_MAX, &notify, portMAX_DELAY);

        //xSemaphoreTake(lpsState_mutex, portMAX_DELAY);
        //uart_fprint(UART_PORT, lps28Data.pressure[0], 2, ',');
        //uart_fprint(UART_PORT, lps28Data.temperature[0], 2, ',');
        //uart_fprint(UART_PORT, lps28Data.pressure[1], 2, ',');
        //uart_fprint(UART_PORT, lps28Data.temperature[1], 2, ',');
        //xSemaphoreGive(lpsState_mutex);
        
        xSemaphoreTake(mcpState_mutex, portMAX_DELAY);
        uart_fprint(UART_PORT, mcp9601Data.tempHot[0], 2, ',');
        uart_fprint(UART_PORT, mcp9601Data.tempCold[0], 2, ',');
        uart_fprint(UART_PORT, mcp9601Data.tempHot[1], 2, ',');
        uart_fprint(UART_PORT, mcp9601Data.tempCold[1], 2, ',');
        uart_fprint(UART_PORT, mcp9601Data.tempHot[2], 2, ',');
        uart_fprint(UART_PORT, mcp9601Data.tempCold[2], 2, '\n');
        xSemaphoreGive(mcpState_mutex);
        
    }
}


int64_t alarm_callback(alarm_id_t id, void *user_data) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(
        sampling_taskHandle, 
        0x01, 
        eSetBits, 
        &xHigherPriorityTaskWoken
    );
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    return SAMPLE_PERIOD;
}

void init_segment_io(void) {
    for (int i = 2; i <= 9; ++i) {
        gpio_init(i);
        gpio_set_dir(i, GPIO_OUT);
        gpio_pull_up(i);
    }
    return ;
}

int main(){
    stdio_init_all();
    cur_state.state = (fsm_state_t)INIT;
    
    uart_init(UART_PORT, 115200);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    // uart_puts(UART_PORT, "Hello UART!\n");
    // printf("Hello, world!\n");
    //  I2C setup and init
    i2c_init(I2C_PORT, I2C_BUS_FREQ);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    init_segment_io(); // init gpio pins to active low outputs for 7-seg disp
    //display_state_start();
    display_state_init();
    // init some gpio pins for oscilloscope debugging
    // gpio_init(GPIO_TOGGLE_PIN);
    // gpio_set_dir(GPIO_TOGGLE_PIN, GPIO_OUT);
    // gpio_init(GPIO_TEST_PIN);
    // gpio_set_dir(GPIO_TEST_PIN, GPIO_OUT);

    pacState_mutex = xSemaphoreCreateMutex();
    mcpState_mutex = xSemaphoreCreateMutex();
    inaState_mutex = xSemaphoreCreateMutex();
    lpsState_mutex = xSemaphoreCreateMutex();
    devState_mutex = xSemaphoreCreateMutex();
    i2cBus_mutex = xSemaphoreCreateMutex();
    configASSERT(i2cBus_mutex != NULL);
    initializeData();
    uint8_t rxdata;
    if (mcp9601_check_available(I2C_PORT, 0x67, &rxdata)) {
        // printf("I2C device found at 0x15 67.\n");
        uart_puts(UART_PORT, "I2C device found at 0x67\n");
    } else {
        uart_puts(UART_PORT, "I2C device not found at 0x67\n");
    }
    if (mcp9601_check_available(I2C_PORT, 0x66, &rxdata)) {
        uart_puts(UART_PORT, "I2C device found at 0x66\n");
    } else {
        uart_puts(UART_PORT, "I2C device not found at 0x66\n");
    }
    if (mcp9601_check_available(I2C_PORT, 0x65, &rxdata)) {
        uart_puts(UART_PORT, "I2C device found at 0x65\n");
    } else {
        uart_puts(UART_PORT, "I2C device not found at 0x65\n");
    }
    for (int i = 0; i < MCP960X_DATA_NUM; ++i) {
        if (!mcp9601_init_device((mcp9601_t*)(mcp_arr+i))) {
            uart_puts(UART_PORT, "MCP-DEV device found \n");
        }
    }


    // check if i2c mcp-device at 0x67 is present
    
    //uint8_t dev_addr = 0x67, buf[2];
    //tcold_res_t cold_res = HIGH_RES;
    //adc_res_t adc_res = RES_18B;
    //uint8_t rxdata;
    //mcp_dev1.i2c = I2C_PORT;
    //mcp_dev1.addr = 0x67;
    //float tcold_temp = -1.0;
    //char stemp[32] = {0};
    //if (mcp9601_check_available(I2C_PORT, dev_addr, &rxdata))
    //{
    //    // printf("I2C device found at 0x15 67.\n");
    //    uart_puts(UART_PORT, "I2C device found at 0x67\n");
    //}
    //
    //if(!(mcp9601_set_device_config(&mcp_dev1, cold_res, adc_res))){
    //    //printf("Unable to set configurations for I2C device at 0x67!\n");
    //    uart_puts(UART_PORT, "Unable to set configurations for I2C device at 0x67!\n");
    //}
    //
    //ina_dev1.i2c = I2C_PORT;
    //ina_dev1.addr = INA228_I2C_ADDR_DEFAULT;
    //float bus_voltage = -1.0f, shunt_voltage = -2.0f;
    /*
    if (ina228_check_available(&ina_dev1) == NO_ERROR)
    {
        uart_puts(UART_PORT, "INA228 Device found at default addr\n");
    }
    // Initialize INA228 device
    if (ina228_init(&ina_dev1) != NO_ERROR){
        uart_puts(UART_PORT, "Unable to initialize INA228 over I2C!\n");
    }
    // INA228: Set Calibrations with max-current and shunt resistor values
    if (ina228_set_calibration(&ina_dev1, INA228_MAX_CURRENT, INA228_SHUNT_RES) != NO_ERROR)
    {
        uart_puts(UART_PORT, "Unable to calibrate INA228 device!\n");
    }
    // INA228: check is able to read bus and shunt voltage.
    if (ina228_read_bus_voltage(&ina_dev1, &bus_voltage) != NO_ERROR)
    {
        uart_puts(UART_PORT, "Unable to read INA228 bus voltage!\n");
    }
    else
    {
        bus_voltage = 85.0f - bus_voltage;
        uart_puts(UART_PORT, "INA228 Bus voltage: ");
        uart_fprint(UART_PORT, bus_voltage, 3, '\n');
    }
    if (ina228_read_shunt_voltage(&ina_dev1, &shunt_voltage) != NO_ERROR)
    {
        uart_puts(UART_PORT, "Unable to read shunt INA228 voltage!\n");
    }
    else
    {
        uart_puts(UART_PORT, "INA228 Shunt voltage: ");
        uart_fprint(UART_PORT, shunt_voltage, 8, '\n');
    }
    */
    
    //pac19_dev1.i2c = I2C_PORT;
    //pac19_dev1.addr = PAC19XX_DEFAULT_ADDR;
    //pac19_dev1.type = PAC_DEVICE_1954;
    //pac19_dev1.shunt_resistor[0] = PAC19XX_SHUNT_RES;
    //pac19_dev1.shunt_resistor[1] = PAC19XX_SHUNT_RES;
    //pac19_dev1.shunt_resistor[2] = PAC19XX_SHUNT_RES;
    //pac19_dev1.shunt_resistor[3] = PAC19XX_SHUNT_RES;
    //float pac_bus_volt = -1.0f, pac_shunt_volt = -2.0f;
    /*
    if (pac19xx_refresh(&pac19_dev1) != NO_ERROR)
    {
        uart_puts(UART_PORT, "Unable to refresh PAC19XX device!\n");
    }
    sleep_ms(2);
    if (pac19xx_read_bus_voltage(&pac19_dev1, 0, &pac_bus_volt) != NO_ERROR)
    {
        uart_puts(UART_PORT, "Unable to read PAC19XX Bus Voltage!\n");
    }
    else
    {
        uart_puts(UART_PORT, "Pac19XX Bus Voltage: ");
        uart_fprint(UART_PORT, pac_bus_volt, 4, '\n');
    }
    if (pac19xx_read_shunt_voltage(&pac19_dev1, 0, &pac_shunt_volt) != NO_ERROR)
    {
        uart_puts(UART_PORT, "Unable to read PAC19XX Shunt Voltage!\n");
    }
    else
    {
        uart_puts(UART_PORT, "Pac19XX Vsense Voltage: ");
        uart_fprint(UART_PORT, (float)(pac_shunt_volt / pac19_dev1.shunt_resistor[0]), 8, '\n');
    }
    /*
    if (pac19xx_refresh(&pac19_dev1) != NO_ERROR)
    {
        uart_puts(UART_PORT, "Unable to refresh PAC19XX device!\n");
    }
    */


    if (lps28dfw_check_available(&lps_dev1) != NO_ERROR){
        uart_puts(UART_PORT, "LPS28DFW-DEV-1 not detected!\n");
    }
    if (lps28dfw_check_available(&lps_dev2) != NO_ERROR){
        uart_puts(UART_PORT, "LPS28DFW-DEV-2 not detected!\n");
    }
    
    if (lps28dfw_soft_reset(&lps_dev1) != NO_ERROR){
        uart_puts(UART_PORT, "DEV-1:Reset Failed!\n");
    }
    
    if (lps28dfw_soft_reset(&lps_dev2) != NO_ERROR){
        uart_puts(UART_PORT, "DEV-2:Reset Failed!\n");
    }

    if (lps28dfw_set_config(&lps_dev1, &cfg1) != NO_ERROR){
        uart_puts(UART_PORT, "DEV-1:Configuration failed!\n");
    }
    
    if (lps28dfw_set_config(&lps_dev2, &cfg2) != NO_ERROR){
        uart_puts(UART_PORT, "DEV-2:Configuration failed!\n");
    }

    while ((!lps28dfw_pressure_ready(&lps_dev1)) && (!lps28dfw_pressure_ready(&lps_dev2))){
        uart_puts(UART_PORT, "In Tight loop\n");
        tight_loop_contents();
    }
    
    uart_puts(UART_PORT, "Initialization Complete!\n");
    //printf("Hello! init complete!\n");
    sleep_ms(5000);
    add_alarm_in_us(SAMPLE_PERIOD, alarm_callback, NULL, false);
    xTaskCreate(fsamplingTask, "sampling-Task", 256, NULL, configMAX_PRIORITIES - 1, &sampling_taskHandle);
    xTaskCreate(fmcp_rtemp_all_task, "mcp-i2c-rt-task", 768, NULL, configMAX_PRIORITIES - 1, &mcp_rtemp_all_taskHandle);
    xTaskCreate(flps28ReadTask, "lps28-i2c-read-task", 850, NULL, configMAX_PRIORITIES - 1, &lps28_read_taskHandle);
    xTaskCreate(fblink_dp_task, "disp-seg-dpblink-task", 250, NULL, configMAX_PRIORITIES - 1, &blink_dp_taskHandle);
    xTaskCreate(fdisp_seg_task, "disp-seg-state-task", 300, NULL, configMAX_PRIORITIES - 1, &dispSeg_taskHandle);
    // xTaskCreate(fpac19VoltReadTask, "pac-i2c-rv-task", 768, NULL, configMAX_PRIORITIES - 1, &pac_vread_taskHandle);
    // xTaskCreate(finaVoltReadTask, "ina-i2c-rv-task", 768, NULL, configMAX_PRIORITIES - 1, &ina_vread_taskHandle);
    xTaskCreate(fuartTransmitTask, "uart-rxtx-task", 820, NULL, configMAX_PRIORITIES - 1, &uart_transmit_taskHandle);
    vTaskStartScheduler();

    // while (1)
    //{    }

    return 0;
}
