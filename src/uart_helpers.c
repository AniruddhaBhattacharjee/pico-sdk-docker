#include "uart_helpers.h"
//#include "hardware/uart.h"
#include <stdint.h>

void float_to_string(float value, char *buffer, int decimals, char delim){
    char *ptr = buffer;
    uint8_t ptr_index = 0;
    if(value < 0.0f){
        // minus sign
        ptr[ptr_index] = '-';
        ptr_index += 1;
        value = -value;
    }
    
    float rounding = 0.5f;
    for(int i = 0; i < decimals; i++){
        rounding *= 0.1f;
    }
    
    value = value + rounding;

    uint32_t int_part = (uint32_t)value;
    float frac_part = value - (float)int_part;
    
    char int_buf[12];
    int i = 0;

    if(int_part == 0){
        int_buf[i++] = '0';
        
    }else{
        while(int_part > 0){
            int_buf[i++] = (int_part % 10) + '0';
            int_part = int_part / 10;
        }
    }

    while(i > 0){
        ptr[ptr_index] = int_buf[--i];
        ptr_index += 1;
    }
    if(decimals > 0){
        ptr[ptr_index] = '.';
        ptr_index += 1;
    
        while(decimals--){
            frac_part *= 10.0f;
            uint32_t digit = (uint32_t)frac_part;
            ptr[ptr_index] = digit + '0';
            ptr_index += 1;
            frac_part -= digit;
        }
    }
    if(delim != '\0'){
        ptr[ptr_index] = delim;
        ptr_index += 1;
    }
    ptr[ptr_index] = '\0';
}

void uart_fprint(uart_inst_t* uart, float value, int decimals, char delim){
    char buffer[32] = {0};
    float_to_string(value, buffer, decimals, delim);
    uart_puts(uart, buffer);
}

void integer_to_string(int value, char *buffer, char delim){
    char *ptr = buffer;
    uint8_t ptr_index = 0;
    if(value < 0){
        ptr[ptr_index] = '-';
        ptr_index += 1;
        value = -value;
    }

    uint32_t int_part = (uint32_t)value;
    
    char int_buf[31];
    int i = 0;

    if(int_part == 0){
        int_buf[i++] = '0';
        
    }else{
        while(int_part > 0){
            int_buf[i++] = (int_part % 10) + '0';
            int_part = int_part / 10;
        }
    }

    while(i > 0){
        ptr[ptr_index] = int_buf[--i];
        ptr_index += 1;
    }

    if(delim != '\0'){
        ptr[ptr_index] = delim;
        ptr_index += 1;
    }
    ptr[ptr_index] = '\0';
}

void uart_iprint(uart_inst_t *uart, int value, char delim){
    char buffer[32] = {0};
    integer_to_string(value, buffer, delim);
    uart_puts(uart, buffer);
}