#include <stdio.h>
#include "pico/stdlib.h"
//#include "pico/time.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"

#include "periph_defs.h"
#include "mcp9601_hal.h"
#include "ina228.h"
#include "uart_helpers.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define SAMPLE_PERIOD 2000
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

// Freertos task functions
void fsamplingTask(void *arg){
    uint32_t notify;
    for(;;){
        xTaskNotifyWait(0, UINT32_MAX, &notify, portMAX_DELAY);
        xTaskNotify(mcp_rtemp_all_taskHandle, 0x01, eSetBits);
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
        if(mcp9601_read_alltemp_reg(I2C_PORT, 0x67, temp_buf, sizeof(temp_buf)) == NO_ERROR){
                tHot_buf[0] = temp_buf[0]; tHot_buf[1] = temp_buf[1];
                tCold_buf[0] = temp_buf[4]; tCold_buf[1] = temp_buf[5];
                //tDelta_buf[0] = temp_buf[2]; tDelta_buf[1] = temp_buf[3];
                THot = mcp9601_convert_to_temp(tHot_buf);
                TCold = mcp9601_convert_to_temp(tCold_buf);
                //TDelta = mcp9601_convert_to_temp(tDelta_buf);
        }else{
            uart_puts(UART_PORT, "ALL TEMP READ ERROR!\n");
        }

        uart_fprint(UART_PORT, THot, 3, ',');
        uart_fprint(UART_PORT, TCold, 3, '\n');
        //uart_fprint(UART_PORT, TDelta, 3, '\n');
        //cur_t = time_us_32();
        //uart_iprint(UART_PORT, cur_t, '\n');
    }
}

void fmcp_rtemp_hc_task(void *arg){
    uint8_t tHot_buf[2], tCold_buf[2];
    float THot = 0.0f, TCold = 0.0f;
    uint32_t notify;
    for(;;){
        xTaskNotifyWait(0, UINT32_MAX, &notify, portMAX_DELAY);
        if (mcp9601_read_tHot_reg(I2C_PORT, 0x67, tHot_buf, sizeof(tHot_buf)) == NO_ERROR){
            THot = mcp9601_convert_to_temp(tHot_buf);
        }
        else{
            uart_puts(UART_PORT, "Hot Temp read Error!\n");
        }
        if (mcp9601_read_tCold_reg(I2C_PORT, 0x67, tCold_buf, sizeof(tCold_buf)) == NO_ERROR){
            TCold = mcp9601_convert_to_temp(tCold_buf);
        }
        else{
            uart_puts(UART_PORT, "Cold Temp read Error!\n");
        }
        uart_fprint(UART_PORT, THot, 3, ',');
        uart_fprint(UART_PORT, TCold, 3, '\n');
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

int main()
{
    stdio_init_all();
    add_alarm_in_us(SAMPLE_PERIOD, alarm_callback, NULL, false);
    xTaskCreate(fsamplingTask, "sampling-Task", 256, NULL, configMAX_PRIORITIES - 1, &sampling_taskHandle);
    xTaskCreate(fmcp_rtemp_all_task, "mcp-i2c-rt-task", 768, NULL, configMAX_PRIORITIES - 1, &mcp_rtemp_all_taskHandle);
    //xTaskCreate(fmcp_rtemp_hc_task, "mcp-i2c-rhc-task", 768, NULL, configMAX_PRIORITIES - 1, &mcp_rtemp_hc, taskHandle);
    
    uart_init(UART_PORT, 115200);
    gpio_set_function(UART1_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART1_RX_PIN, GPIO_FUNC_UART);
    //uart_puts(UART_PORT, "Hello UART!\n");
    //printf("Hello, world!\n");
    
    i2c_init(I2C_PORT, I2C_BUS_FREQ);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    gpio_init(GPIO_TOGGLE_PIN);
    gpio_set_dir(GPIO_TOGGLE_PIN, GPIO_OUT);
    gpio_init(GPIO_TEST_PIN);
    gpio_set_dir(GPIO_TEST_PIN, GPIO_OUT);
    // check if i2c device at 0x67 is present
    uint8_t dev_addr = 0x67, buf[2];
    tcold_res_t cold_res = HIGH_RES;
    adc_res_t adc_res = RES_18B;
    uint8_t *rxdata;
    float tcold_temp = -1.0;
    char stemp[32] = {0};
    if(mcp9601_check_available(I2C_PORT, dev_addr, rxdata)){
        //printf("I2C device found at 0x67.\n");
        uart_puts(UART_PORT, "I2C device found at 0x67\n");
    }
    
    if(!(mcp9601_set_device_config(I2C_PORT, dev_addr, cold_res, adc_res))){
        //printf("Unable to set configurations for I2C device at 0x67!\n");
        uart_puts(UART_PORT, "Unable to set configurations for I2C device at 0x67!\n");
    }
    ina228_t ina_dev1;
    ina_dev1.i2c = I2C_PORT;
    ina_dev1.addr = INA228_I2C_ADDR_DEFAULT;
    if (ina228_init(&ina_dev1) != NO_ERROR){
        uart_puts(UART_PORT, "Unable to initialize INA228 over I2C!\n");
    }
    //if (ina228_set_calibration(&ina_dev1, ))
    sleep_ms(2000);
    vTaskStartScheduler();
    while(1){
    /*
    uint8_t temp_buf[6], tHot_buf[2], tCold_buf[2], tDelta_buf[2];
    float THot = 0.0f, TCold = 0.0f, TDelta = 0.0f;
    //uint32_t start = time_us_32();
    if (mcp9601_read_alltemp_reg(I2C_PORT, dev_addr, temp_buf, sizeof(temp_buf)) == NO_ERROR){
        tHot_buf[0] = temp_buf[0]; tHot_buf[1] = temp_buf[1];
        tCold_buf[0] = temp_buf[4]; tCold_buf[1] = temp_buf[5];
        tDelta_buf[0] = temp_buf[2]; tDelta_buf[1] = temp_buf[3];
        THot = mcp9601_convert_to_temp(tHot_buf);
        TCold = mcp9601_convert_to_temp(tCold_buf);
        TDelta = mcp9601_convert_to_temp(tDelta_buf);
    }
    uart_fprint(UART_PORT, THot, 3, ',');
    uart_fprint(UART_PORT, TCold, 3, ',');
    //uart_fprint(UART_PORT, TDelta, 3, '\n');
    //uint32_t end = time_us_32();
    //uart_iprint(UART_PORT, (uint32_t)(end - start), '\n');
    //sleep_ms(2000);
    //vTaskStartScheduler();
    
    //while(1){
        /*
        gpio_put(GPIO_TOGGLE_PIN, 1);
        sleep_ms(200);
        gpio_put(GPIO_TOGGLE_PIN, 0);
        sleep_ms(200);
        printf("Loop.\n");
        if (mcp9601_read_alltemp_reg(I2C_PORT, dev_addr, temp_buf, sizeof(temp_buf)) == NO_ERROR){
            tHot_buf[0] = temp_buf[0]; tHot_buf[1] = temp_buf[1];
            tCold_buf[0] = temp_buf[4]; tCold_buf[1] = temp_buf[5];
            tDelta_buf[0] = temp_buf[2]; tDelta_buf[1] = temp_buf[3];
            THot = mcp9601_convert_to_temp(tHot_buf);
            TCold = mcp9601_convert_to_temp(tCold_buf);
            TDelta = mcp9601_convert_to_temp(tDelta_buf);
        }
        uart_fprint(UART_PORT, THot, 3, ',');
        uart_fprint(UART_PORT, TCold, 3, ',');
        uart_fprint(UART_PORT, TDelta, 3, '\n');
        //uart_puts(UART_PORT, "UART loop msg.");
        */
    }

    return 0;
}
