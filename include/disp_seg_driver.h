#ifndef DISP_SEG_DRIVER_H
#define DISP_SEG_DRIVER_H

#define SEG_DP_MASK (uint32_t)(0x01 << SEG_DP)

void display_state_start(void);
void display_state_stop(void);
void display_state_init(void);
void display_state_measure(void);
void display_state_error(void);
// void blink_dp(void* arg);
void display_state_all(void);

#endif