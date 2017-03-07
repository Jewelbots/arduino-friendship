#ifndef BUTTON_HANDLER_H_
#define BUTTON_HANDLER_H_
#include <stdint.h>
void buttons_init(void);
void button_tick_increment(void);
#ifdef DEMO_MODE
void demo_run(void);
void pmic_int_pin_handler(nrf_drv_gpiote_pin_t pin,
                          nrf_gpiote_polarity_t action);
#endif
#endif
