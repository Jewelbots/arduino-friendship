#ifndef __LED_DRIVER_H__
#define __LED_DRIVER_H__
#include <stdbool.h>
#include "jewelbot_types.h"



void services_init(void);
void led_test_run(void);
void enable_led(void);
void disable_led(void);
void set_led_state_handler(led_cmd_t *state);
extern time_t jwbs_tod; // current time of day
extern time_t jwbs_last_sync;

#endif
