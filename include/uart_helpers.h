#ifndef UART_HELPERS_H
#define UART_HELPERS_H
#include "hardware/uart.h"

void float_to_string(float value, char *buffer, int decimals, char delim);
void uart_fprint(uart_inst_t *uart, float value, int decimals, char delim);
void integer_to_string(int value, char *buffer, char delim);
void uart_iprint(uart_inst_t *uart, int value, char delim);
#endif