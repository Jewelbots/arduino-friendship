#ifndef __LED_SEQUENCE_H__
#define __LED_SEQUENCE_H__

#include "fsm.h"
#include <stdint.h>

void led_indicate_error(void);
void breathe_all(uint8_t color);
void clear_led(void);
void boot_up_led_sequence(void);
void signal_power_down(void);
void signal_enter_pairing_mode(void);
void logo_breathe(void);
void breathe_friends(void);
void show_friends(uint8_t *colors, uint8_t num_of_colors);
void led_sequence_buffer_push(uint8_t color_index);
void cycle_through_all_colors(jewelbot_t *me);
void cycle_through_present_colors(buf_array_t *colors);
void flash_white(void);
void led_indicate_bonding_state(void);
bool friends_to_display(void);
void initialize_led_buffer(void);
void led_indicate_charging_state(bool is_charging);
void show_all(uint8_t color_index);
void get_color_index_for_color(uint32_t color, uint8_t *color_index);
cbuf_t *get_led_buffer(void);
uint8_t get_current_msg_color_cycle(void);
void reset_current_msg_color_cycle(void);
uint8_t *get_current_msg_iterator(void);
void set_current_msg_iterator(uint8_t new_value);
void reset_current_msg_iterator(void);
void led_indicate_connected(void);
void signal_friend_success(uint8_t color_index);

bool see_red_friends(void);
bool see_green_friends(void);
bool see_blue_friends(void);
bool see_cyan_friends(void);

#endif
