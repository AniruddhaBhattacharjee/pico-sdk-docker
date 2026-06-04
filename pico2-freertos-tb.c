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

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define SAMPLE_PERIOD 4000 // in microseconds
/*
// Data will be copied from src to dst
const char src[] = "Hello, world! (from DMA)";
char dst[count_of(src)];
*/
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
// Freertos Semaphore
static SemaphoreHandle_t pacState_mutex;
static SemaphoreHandle_t mcpState_mutex;
static SemaphoreHandle_t inaState_mutex;

// declare sensor data objects
mcp960xData_t mcp9601Data;
pac19xxData_t pac1954Data;
ina228Data_t ina228Data;

// declare sensor device structs
mcp9601_t mcp_dev1;
pac19xx_t pac19_dev1;
ina228_t ina_dev1;

// setup functions
void initializeData(){
    int i;
    mcp9601Data.index = 0;
    for (i = 0; i < MCP960X_DATA_NUM; ++i){
        mcp9601Data.tempCold[i] = 0.0f;
        mcp9601Data.tempHot[i] = 0.0f;
    }
    ina228Data.index = 0;
    for (i = 0; i < INA228_DATA_NUM; ++i){
        ina228Data.bus_volt[i] = 0.0f;
        ina228Data.shunt_volt[i] = 0.0f;
    }
    pac1954Data.index = 0;
    for (i = 0; i < PAC19XX_DATA_NUM; ++i){
        pac1954Data.bus_volt[i] = 0.0f;
        pac1954Data.shunt_volt[i] = 0.0f;
    }
    return;
}

// Freertos task functions
void fsamplingTask(void *arg){
    uint32_t notify;
    for(;;){
        xTaskNotifyWait(0, UINT32_MAX, &notify, portMAX_DELAY);
        xTaskNotify(mcp_rtemp_all_taskHandle, 0x01, eSetBits);
        // xTaskNotify(pac_vread_taskHandle, 0x01, eSetBits);
        // xTaskNotify(ina_vread_taskHandle, 0x01, eSetBits);
        vTaskDelay(pdMS_TO_TICKS(1));
        xTaskNotify(uart_transmit_taskHandle, 0x01, eSetBits);
    }
}

void fmcp_rtemp_all_task(void *arg){
    uint8_t temp_buf[6], tHot_buf[2], tCold_buf[2], tDelta_buf[2];
    float THot = 0.0f, TCold = 0.0f, TDelta = 0.0f;
    uint8_t pin_state = 0;
    uint32_t notify;
    //uint32_t cur_t;
    for(;;){
        xTaskNotifyWait(0, UINT32_MAX, &notify, portMAX_DELAY);
        pin_state = ~(pin_state);
        gpio_put(GPIO_TOGGLE_PIN, pin_state);
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
        //uart_fprint(UART_PORT, THot, 3, ',');
        //uart_fprint(UART_PORT, TCold, 3, '\n');
    }
}

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

void fpac19VoltReadTask(void *arg)
{
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

void finaVoltReadTask(void *arg)
{
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

void fuartTransmitTask(void *arg)
{
    uint32_t notify;
    for (;;)
    {
        xTaskNotifyWait(0, UINT32_MAX, &notify, portMAX_DELAY);

        xSemaphoreTake(mcpState_mutex, portMAX_DELAY);
        uart_fprint(UART_PORT, mcp9601Data.tempHot[0], 3, ',');
        uart_fprint(UART_PORT, mcp9601Data.tempCold[0], 3, '\n');
        xSemaphoreGive(mcpState_mutex);
        /*
        xSemaphoreTake(pacState_mutex, portMAX_DELAY);
        uart_fprint(UART_PORT, pac1954Data.bus_volt[0], 3, ',');
        uart_fprint(UART_PORT, pac1954Data.shunt_volt[0], 6, ',');
        xSemaphoreGive(pacState_mutex);

        xSemaphoreTake(inaState_mutex, portMAX_DELAY);
        uart_fprint(UART_PORT, ina228Data.bus_volt[0], 3, ',');
        uart_fprint(UART_PORT, ina228Data.shunt_volt[0], 6, '\n');
        xSemaphoreGive(inaState_mutex);
        */
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

int main(){
    stdio_init_all();

    uart_init(UART_PORT, 115200);
    gpio_set_function(UART1_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART1_RX_PIN, GPIO_FUNC_UART);
    // uart_puts(UART_PORT, "Hello UART!\n");
    // printf("Hello, world!\n");
    //  I2C setup and init
    i2c_init(I2C_PORT, I2C_BUS_FREQ);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // init some gpio pins for oscilloscope debugging
    gpio_init(GPIO_TOGGLE_PIN);
    gpio_set_dir(GPIO_TOGGLE_PIN, GPIO_OUT);
    gpio_init(GPIO_TEST_PIN);
    gpio_set_dir(GPIO_TEST_PIN, GPIO_OUT);

    pacState_mutex = xSemaphoreCreateMutex();
    mcpState_mutex = xSemaphoreCreateMutex();
    inaState_mutex = xSemaphoreCreateMutex();

    initializeData();

    // check if i2c mcp-device at 0x67 is present
    uint8_t dev_addr = 0x67, buf[2];
    tcold_res_t cold_res = HIGH_RES;
    adc_res_t adc_res = RES_18B;
    uint8_t rxdata;
    mcp_dev1.i2c = I2C_PORT;
    mcp_dev1.addr = 0x67;
    float tcold_temp = -1.0;
    char stemp[32] = {0};
    if (mcp9601_check_available(I2C_PORT, dev_addr, &rxdata))
    {
        // printf("I2C device found at 0x15 67.\n");
        uart_puts(UART_PORT, "I2C device found at 0x67\n");
    }

    if(!(mcp9601_set_device_config(&mcp_dev1, cold_res, adc_res))){
        //printf("Unable to set configurations for I2C device at 0x67!\n");
        uart_puts(UART_PORT, "Unable to set configurations for I2C device at 0x67!\n");
    }

    ina_dev1.i2c = I2C_PORT;
    ina_dev1.addr = INA228_I2C_ADDR_DEFAULT;
    float bus_voltage = -1.0f, shunt_voltage = -2.0f;
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

    pac19_dev1.i2c = I2C_PORT;
    pac19_dev1.addr = PAC19XX_DEFAULT_ADDR;
    pac19_dev1.type = PAC_DEVICE_1954;
    pac19_dev1.shunt_resistor[0] = PAC19XX_SHUNT_RES;
    pac19_dev1.shunt_resistor[1] = PAC19XX_SHUNT_RES;
    pac19_dev1.shunt_resistor[2] = PAC19XX_SHUNT_RES;
    pac19_dev1.shunt_resistor[3] = PAC19XX_SHUNT_RES;
    float pac_bus_volt = -1.0f, pac_shunt_volt = -2.0f;
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
    sleep_ms(10000);

    add_alarm_in_us(SAMPLE_PERIOD, alarm_callback, NULL, false);
    xTaskCreate(fsamplingTask, "sampling-Task", 256, NULL, configMAX_PRIORITIES - 1, &sampling_taskHandle);
    xTaskCreate(fmcp_rtemp_all_task, "mcp-i2c-rt-task", 768, NULL, configMAX_PRIORITIES - 1, &mcp_rtemp_all_taskHandle);
    // xTaskCreate(fpac19VoltReadTask, "pac-i2c-rv-task", 768, NULL, configMAX_PRIORITIES - 1, &pac_vread_taskHandle);
    // xTaskCreate(finaVoltReadTask, "ina-i2c-rv-task", 768, NULL, configMAX_PRIORITIES - 1, &ina_vread_taskHandle);
    xTaskCreate(fuartTransmitTask, "uart-rxtx-task", 768, NULL, configMAX_PRIORITIES - 1, &uart_transmit_taskHandle);
    vTaskStartScheduler();

    // while (1)
    //{    }

    return 0;
}
