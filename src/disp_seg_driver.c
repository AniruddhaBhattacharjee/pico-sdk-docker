#include "disp_seg_driver.h"
#include "periph_defs.h"
#include "hardware/gpio.h"

void display_state_start() {
    // a, f, g, c, d : turn on
    gpio_put(SEG_A, 0);
    gpio_put(SEG_F, 0);
    gpio_put(SEG_G, 0);
    gpio_put(SEG_C, 0);
    gpio_put(SEG_D, 0);
    // turn off
    gpio_put(SEG_B, 1);
    gpio_put(SEG_E, 1);
    return ;
}

void display_state_stop(void) {
    // turn on: G
    gpio_put(SEG_G, 0);
    // turn off
    gpio_put(SEG_A, 1);
    gpio_put(SEG_B, 1);
    gpio_put(SEG_C, 1);
    gpio_put(SEG_D, 1);
    gpio_put(SEG_E, 1);
    gpio_put(SEG_F, 1);
    return ;

}

void display_state_init(void) {
    // turn on: B, c
    gpio_put(SEG_B, 0);
    gpio_put(SEG_C, 0);
    // turn off
    gpio_put(SEG_A, 1);
    gpio_put(SEG_D, 1);
    gpio_put(SEG_E, 1);
    gpio_put(SEG_F, 1);
    gpio_put(SEG_G, 1);
    return ;
}

void display_state_measure(void) {
    // turn on: a, f, b, e, c, g
    gpio_put(SEG_A, 0);
    gpio_put(SEG_B, 0);
    gpio_put(SEG_C, 0);
    gpio_put(SEG_E, 0);
    gpio_put(SEG_F, 0);
    gpio_put(SEG_G, 0);
    // turn off
    gpio_put(SEG_D, 1);
    return ;
}

void display_state_error(void) {
    // turn on: 
    gpio_put(SEG_A, 0);
    gpio_put(SEG_F, 0);
    gpio_put(SEG_G, 0);
    gpio_put(SEG_E, 0);
    gpio_put(SEG_D, 0);
    // turn off
    gpio_put(SEG_B, 1);
    gpio_put(SEG_C, 1);
    return ;
}

void display_state_all(void) {
    // turn on all
    gpio_put(SEG_A, 0);
    gpio_put(SEG_B, 0);
    gpio_put(SEG_C, 0);
    gpio_put(SEG_D, 0);
    gpio_put(SEG_E, 0);
    gpio_put(SEG_F, 0);
    gpio_put(SEG_G, 0);
    return ;

}
