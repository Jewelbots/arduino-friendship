#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "app_timer.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_soc.h"

#include "ble_central_event_handler.h"
#include "cbuf.h"
#include "common_defines.h"
#include "fsm.h"
#include "haptics_driver.h"
#include "jewelbot_gpio.h"
#include "led_driver.h"
#include "led_sequence.h"

#define RED 0x3F0000
#define GREEN 0x003F00
#define BLUE 0x00003F
#define YELLOW 0x3F3F00
#define CYAN 0x003F3F
#define MAGENTA 0x3F003F
#define PINK 0x3F0524
#define CHARTREUSE 0x1F3F00

#define LED_BUFFER_SIZE 16
#define SENTINEL_VALUE 0xEE
color_rgb_t colors_array[] = {
    {.word = BLUE}, {.word = GREEN}, {.word = RED}, {.word = YELLOW}};

CIRCBUF_DEF(led_cb,
            LED_BUFFER_SIZE); // move to a much higher number for testing 17?
void led_sequence_buffer_push(uint8_t color_index) {
  c_buf_code_t err_code = c_buf_push(&led_cb, color_index);
}

cbuf_t *get_led_buffer() { return &led_cb; }
bool friends_to_display() { return has_elements(&led_cb); }
static uint8_t current_msg_color_cycle = 0;
static uint8_t msg_iterator = 0;
uint8_t get_current_msg_color_cycle(void) { return current_msg_color_cycle; }
static void set_current_msg_color_cycle(uint8_t new_value) {
  current_msg_color_cycle = new_value;
}
uint8_t *get_current_msg_iterator(void) { return &msg_iterator; }
void set_current_msg_iterator(uint8_t new_value) { msg_iterator = new_value; }
void reset_current_msg_iterator() { msg_iterator = 0; }
void reset_current_msg_color_cycle(void) { current_msg_color_cycle = 0; }
static void show_red() {
  led_cmd_t led = {0, 0x3F, 0x00, 0x00, 1};
  led_cmd_t led2 = {1, 0x3F, 0x00, 0x00, 1};
  led_cmd_t led3 = {2, 0x3F, 0x00, 0x00, 1};
  led_cmd_t led4 = {3, 0x3F, 0x00, 0x00, 1};
  set_led_state_handler(&led);

  set_led_state_handler(&led2);

  set_led_state_handler(&led3);

  set_led_state_handler(&led4);
}
void led_indicate_error() {
  enable_led();
  clear_led();
  show_red();
  nrf_delay_ms(200);
  clear_led();
  nrf_delay_ms(200);
  show_red();
  nrf_delay_ms(200);
  clear_led();
  nrf_delay_ms(200);
  show_red();
  nrf_delay_ms(200);
  clear_led();
  nrf_delay_ms(200);
  disable_led();
}
void initialize_led_buffer() {
  uint8_t data;
  for (uint8_t i = 0; i <= LED_BUFFER_SIZE; i++) {
    c_buf_init(&led_cb, SENTINEL_VALUE);
    c_buf_pop(&led_cb, &data);
  }
}

void signal_power_down() {
  enable_led();
  clear_led();
  led_cmd_t red = {0, 0x3F, 0x00, 0x00, 1};
  led_cmd_t orange = {1, 0x3F, 0x1C, 0x00, 1};
  led_cmd_t yellow = {2, 0x3F, 0x3F, 0x00, 1};
  led_cmd_t green = {3, 0x00, 0x3F, 0x00, 1};
  led_cmd_t blue = {0, 0x00, 0x00, 0x3F, 1};
  led_cmd_t violet = {1, 0x1F, 0x00, 0x3F, 1};
  led_cmd_t pink = {2, 0x3F, 0x1A, 0x3A, 1};
  led_cmd_t magenta = {3, 0x3F, 0x0C, 0x3F, 1};

  int delay = 65000;
  for (int i = 2; i > 0; i--) {
    set_led_state_handler(&red);
    nrf_delay_us(delay);
    set_led_state_handler(&orange);
    nrf_delay_us(delay);
    set_led_state_handler(&yellow);
    nrf_delay_us(delay);
    set_led_state_handler(&green);
    nrf_delay_us(delay);
    set_led_state_handler(&blue);
    nrf_delay_us(delay);
    set_led_state_handler(&violet);
    nrf_delay_us(delay);
    set_led_state_handler(&pink);
    nrf_delay_us(delay);
    set_led_state_handler(&magenta);
    nrf_delay_us(delay);
  }

	led_cmd_t led1 = {3, 0x3F, 0x0C, 0x3F, 1};
  led_cmd_t led2 = {1, 0x22, 0x00, 0x3F, 1};
  led_cmd_t led3 = {2, 0x3F, 0x1C, 0x3A, 1};
  led_cmd_t led4 = {0, 0x00, 0x00, 0x3F, 1};
	
	//  quickly fade out the colors
  for (uint8_t i = 11; i > 0; i--) {
    led1.r = led1.r - 5;
    led2.r = led2.r - 3;
    led3.r = led3.r - 5;
    led4.r = led4.r - 0;
    led1.g = led1.g - 1;
    led2.g = led2.g - 0;
    led3.g = led3.g - 1;
    led4.g = led4.g - 0;
    led1.b = led1.b - 5;
    led2.b = led2.b - 5;
    led3.b = led3.b - 5;
    led4.b = led4.b - 5;

    set_led_state_handler(&led1);
    set_led_state_handler(&led2);
    set_led_state_handler(&led3);
    set_led_state_handler(&led4);
    nrf_delay_us(95000);
  }
	
  clear_led();
  disable_led();
}

void signal_enter_pairing_mode(void) {
  for (int i = 0; i < 4; i = i + 1) {
    led_cmd_t white = {i, 0x3F, 0x0C, 0x3F, 1};
    set_led_state_handler(&white);
  }
}

void flash_white(void) {
  enable_led();
  clear_led();
  led_cmd_t led1 = {0, 0x3F, 0x3F, 0x3F, 0};
  led_cmd_t led2 = {1, 0x3F, 0x3F, 0x3F, 0};
  led_cmd_t led3 = {2, 0x3F, 0x3F, 0x3F, 0};
  led_cmd_t led4 = {3, 0x3F, 0x3F, 0x3F, 0};
  set_led_state_handler(&led1);
  set_led_state_handler(&led2);
  set_led_state_handler(&led3);
  set_led_state_handler(&led4);
}

void led_indicate_bonding_state(void) {
  enable_led();
  clear_led();
  led_cmd_t led1 = {0, 0x00, 0x3F, 0x00, 0};
  led_cmd_t led2 = {1, 0x00, 0x3F, 0x00, 0};
  led_cmd_t led3 = {2, 0x00, 0x3F, 0x00, 0};
  led_cmd_t led4 = {3, 0x00, 0x3F, 0x00, 0};
  set_led_state_handler(&led1);
  set_led_state_handler(&led2);
  set_led_state_handler(&led3);
  set_led_state_handler(&led4);
}

void led_indicate_connected(void) {
  enable_led();
  clear_led();
  led_cmd_t led1 = {0, 0x00, 0x3F, 0x00, 0};
  led_cmd_t led2 = {1, 0x00, 0x3F, 0x00, 0};
  led_cmd_t led3 = {2, 0x00, 0x3F, 0x00, 0};
  led_cmd_t led4 = {3, 0x00, 0x3F, 0x00, 0};
  set_led_state_handler(&led1);
  set_led_state_handler(&led2);
  set_led_state_handler(&led3);
  set_led_state_handler(&led4);
}
#define MAX_NUMBER_OF_LEDS 3 // 4 leds, zero based
void cycle_through_all_colors(jewelbot_t *me) {
  uint8_t current_cycle = get_current_cycle();
  if (current_cycle > MAX_NUMBER_OF_LEDS) {
    set_current_cycle(0);
  }
  enable_led();
  clear_led();
  led_cmd_t led = {0, 0x00, 0x00, 0x00, 0};
  rgb_t c;

  led.led = current_cycle;
  c = colors_array[current_cycle].color;

  led.r = c.r;
  led.g = c.g;
  led.b = c.b;
  set_led_state_handler(&led);
  // rgb_t rgb = { led.r, led.g, led.b };
  set_current_color(current_cycle);
}

void reset_cycle(void) { current_msg_color_cycle = 0; }

//{ 1 };
void cycle_through_present_colors(buf_array_t *colors) {
  set_current_msg_color_cycle(colors->buffer[msg_iterator]);
  static uint8_t led_cycle;
  if (led_cycle > MAX_NUMBER_OF_LEDS) {
    led_cycle = 0;
  }
  enable_led();
  clear_led();
  led_cmd_t led = {0, 0x00, 0x00, 0x00, 0};
  rgb_t c;

  led.led = led_cycle;
  c = colors_array[colors->buffer[msg_iterator]].color;
  led.r = c.r;
  led.g = c.g;
  led.b = c.b;
  set_led_state_handler(&led);
  set_current_msg_color_cycle(colors->buffer[msg_iterator]);
  led_cycle++;
  if (msg_iterator >= colors->buf_length - 1) {
    //set_current_msg_color_cycle(colors->buffer[0]);
    msg_iterator = 0;
  } else {
    msg_iterator++;
  }
}
static void set_color_state(led_cmd_t *leds[], uint8_t *colors,
                            uint8_t num_of_colors) {
  if (num_of_colors >= 1) {
    leds[0]->r = colors_array[colors[0]].color.r;
    leds[1]->r = colors_array[colors[0]].color.r;
    leds[2]->r = colors_array[colors[0]].color.r;
    leds[3]->r = colors_array[colors[0]].color.r;
    leds[0]->g = colors_array[colors[0]].color.g;
    leds[1]->g = colors_array[colors[0]].color.g;
    leds[2]->g = colors_array[colors[0]].color.g;
    leds[3]->g = colors_array[colors[0]].color.g;
    leds[0]->b = colors_array[colors[0]].color.b;
    leds[1]->b = colors_array[colors[0]].color.b;
    leds[2]->b = colors_array[colors[0]].color.b;
    leds[3]->b = colors_array[colors[0]].color.b;
  }
  if (num_of_colors >= 2) {
    leds[1]->r = colors_array[colors[1]].color.r;
    leds[2]->r = colors_array[colors[1]].color.r;

    leds[1]->g = colors_array[colors[1]].color.g;
    leds[2]->g = colors_array[colors[1]].color.g;

    leds[1]->b = colors_array[colors[1]].color.b;
    leds[2]->b = colors_array[colors[1]].color.b;
  }
  if (num_of_colors >= 3) {
    leds[3]->r = colors_array[colors[2]].color.r;
    leds[2]->r = 0;

    leds[3]->g = colors_array[colors[2]].color.g;
    leds[2]->g = 0;

    leds[3]->b = colors_array[colors[2]].color.b;
    leds[2]->b = 0;
  }
  if (num_of_colors == 4) {
    leds[2]->r = colors_array[colors[3]].color.r;
    leds[2]->g = colors_array[colors[3]].color.g;
    leds[2]->b = colors_array[colors[3]].color.b;
  }

  set_led_state_handler(leds[0]);
  set_led_state_handler(leds[1]);
  set_led_state_handler(leds[2]);
  set_led_state_handler(leds[3]);
}

void breathe_friends() {
  uint8_t color_index;
  uint8_t colors[] = {SENTINEL_VALUE, SENTINEL_VALUE, SENTINEL_VALUE,
                      SENTINEL_VALUE};
  c_buf_code_t err_code = BUF_SUCCESS;
  uint8_t num_of_colors = 0;
  while (err_code == BUF_SUCCESS) {
    if (num_of_colors >= 4) {
      break; // todo: better way
    }
    err_code = c_buf_pop(&led_cb, &color_index);
    if (err_code == BUF_SUCCESS && color_index != SENTINEL_VALUE) {
      colors[num_of_colors] = color_index;
      num_of_colors++;
    }
  }
  if (err_code != BUF_SUCCESS) {
  }
  if (num_of_colors > 0) {
    if (num_of_colors > 4) {
    }
    NRF_LOG_DEBUG("*");
    show_friends(colors, num_of_colors);
  } else {
    uint32_t evt_id = get_current_sys_evt();
    NRF_LOG_PRINTF_DEBUG("%u\r\n", evt_id);
    remove_from_msg_buffer(); // TODO: Should be a 1 for 1 remove with colors
                              // that are gone
  }
  num_of_colors = 0;
}

void show_friends(uint8_t *colors, uint8_t num_of_colors) {
  led_cmd_t led1 = {3, 0x00, 0x00, 0x00, 0};
  led_cmd_t led2 = {1, 0x00, 0x00, 0x00, 0};
  led_cmd_t led3 = {2, 0x00, 0x00, 0x00, 0};
  led_cmd_t led4 = {0, 0x00, 0x00, 0x00, 0};
  led_cmd_t *leds[4] = {&led1, &led2, &led3, &led4};

  set_color_state(leds, colors, num_of_colors);
}

void logo_breathe(void) {
  enable_led();
  clear_led();
  // Fade in the logo colors quickly
  led_cmd_t led1 = {3, 0x00, 0x00, 0x00, 0};
  led_cmd_t led2 = {1, 0x00, 0x00, 0x00, 0};
  led_cmd_t led3 = {2, 0x00, 0x00, 0x00, 0};
  led_cmd_t led4 = {0, 0x00, 0x00, 0x00, 0};
  for (uint8_t i = 0; i < 12; i++) {
    led1.r = led1.r + 5;
    led2.r = led2.r + 0;
    led3.r = led3.r + 0;
    led4.r = led4.r + 0;
    led1.g = led1.g + 0;
    led2.g = led2.g + 2;
    led3.g = led3.g + 2;
    led4.g = led4.g + 2;
    led1.b = led1.b + 0;
    led2.b = led2.b + 1;
    led3.b = led3.b + 1;
    led4.b = led4.b + 1;

    set_led_state_handler(&led1);
    set_led_state_handler(&led2);
    set_led_state_handler(&led3);
    set_led_state_handler(&led4);
    nrf_delay_us(40000);
  }

  //  Rotate the logo colors around the face
  led_cmd_t red_hold = {3, 0x3F, 0x00, 0x00, 0};
  led_cmd_t teal_hold1 = {1, 0x00, 0x18, 0x0C, 0};
  led_cmd_t teal_hold2 = {2, 0x00, 0x18, 0x0C, 0};
  led_cmd_t teal_hold3 = {0, 0x00, 0x18, 0x0C, 0};
  set_led_state_handler(&red_hold);
  set_led_state_handler(&teal_hold1);
  set_led_state_handler(&teal_hold2);
  set_led_state_handler(&teal_hold3);

  uint8_t red_led[] = {3, 0, 1, 2};
  uint8_t teal_led[] = {2, 3, 0, 1};
  for (uint8_t i = 0; i < 6; i++) {
    for (uint8_t j = 0; j < 4; j++) {
      led_cmd_t red_ch = {red_led[j], 0x3F, 0x00, 0x00, 0};
      led_cmd_t teal_ch = {teal_led[j], 0x00, 0x24, 0x18, 0};
      set_led_state_handler(&red_ch);
      set_led_state_handler(&teal_ch);
      nrf_delay_us(80000);
    }
#ifdef REAL_JEWELBOT
// haptics_test_run1();
#endif
  }

  //  quickly fade out the colors
  for (uint8_t i = 12; i > 0; i--) {
    led1.r = led1.r - 5;
    led2.r = led2.r - 0;
    led3.r = led3.r - 0;
    led4.r = led4.r - 0;
    led1.g = led1.g - 0;
    led2.g = led2.g - 2;
    led3.g = led3.g - 2;
    led4.g = led4.g - 2;
    led1.b = led1.b - 0;
    led2.b = led2.b - 1;
    led3.b = led3.b - 1;
    led4.b = led4.b - 1;

    set_led_state_handler(&led1);
    set_led_state_handler(&led2);
    set_led_state_handler(&led3);
    set_led_state_handler(&led4);
    nrf_delay_us(40000);
  }
  // nrf_delay_us(10000);
  clear_led();
  disable_led();
}

void breathe_all(uint8_t color) {
  enable_led();
  clear_led();

  // enable_led();
  led_cmd_t led1 = {3, 0x00, 0x00, 0x00, 0};
  led_cmd_t led2 = {1, 0x00, 0x00, 0x00, 0};
  led_cmd_t led3 = {2, 0x00, 0x00, 0x00, 0};
  led_cmd_t led4 = {0, 0x00, 0x00, 0x00, 0};
  for (uint8_t i = 0; i < 31; i++) {
    if (color == 1) {
      led1.r = led1.r + 1;
      led2.r = led2.r + 1;
      led3.r = led3.r + 1;
      led4.r = led4.r + 1;
    }
    if (color == 2) {
      led1.g = led1.g + 1;
      led2.g = led2.g + 1;
      led3.g = led3.g + 1;
      led4.g = led4.g + 1;
    }
    if (color == 3) {
      led1.b = led1.b + 1;
      led2.b = led2.b + 1;
      led3.b = led3.b + 1;
      led4.b = led4.b + 1;
    }
    if (color == 4) {
      led1.r = led1.r + 1;
      led2.r = led2.r + 1;
      led3.r = led3.r + 1;
      led4.r = led4.r + 1;
      led1.b = led1.b + 1;
      led2.b = led2.b + 1;
      led3.b = led3.b + 1;
      led4.b = led4.b + 1;
    }
    set_led_state_handler(&led1);
    set_led_state_handler(&led2);
    set_led_state_handler(&led3);
    set_led_state_handler(&led4);
    nrf_delay_us(30000);
  }
  nrf_delay_us(100);
  for (uint8_t i = 31; i > 0; i--) {
    if (color == 1) {
      led1.r = led1.r - 1;
      led2.r = led2.r - 1;
      led3.r = led3.r - 1;
      led4.r = led4.r - 1;
    }
    if (color == 2) {
      led1.g = led1.g - 1;
      led2.g = led2.g - 1;
      led3.g = led3.g - 1;
      led4.g = led4.g - 1;
    }
    if (color == 3) {
      led1.b = led1.b - 1;
      led2.b = led2.b - 1;
      led3.b = led3.b - 1;
      led4.b = led4.b - 1;
    }
    if (color == 4) {
      led1.r = led1.r - 1;
      led2.r = led2.r - 1;
      led3.r = led3.r - 1;
      led4.r = led4.r - 1;
      led1.b = led1.b - 1;
      led2.b = led2.b - 1;
      led3.b = led3.b - 1;
      led4.b = led4.b - 1;
    }
    set_led_state_handler(&led1);
    set_led_state_handler(&led2);
    set_led_state_handler(&led3);
    set_led_state_handler(&led4);
    nrf_delay_us(30000);
  }
  nrf_delay_us(10000);
  clear_led();
  disable_led();
}

void clear_led() {
  led_cmd_t clear1 = {0, 0x00, 0x00, 0x00, 0};
  led_cmd_t clear2 = {1, 0x00, 0x00, 0x00, 0};
  led_cmd_t clear3 = {2, 0x00, 0x00, 0x00, 0};
  led_cmd_t clear4 = {3, 0x00, 0x00, 0x00, 0};
  // enable_led();
  set_led_state_handler(&clear1);
  set_led_state_handler(&clear2);
  set_led_state_handler(&clear3);
  set_led_state_handler(&clear4);
  // disable_led();
}

void boot_up_led_sequence(void) {
  enable_led();
  clear_led();
  led_cmd_t red = {0, 0x3F, 0x00, 0x00, 1};
  led_cmd_t orange = {1, 0x3F, 0x1C, 0x00, 1};
  led_cmd_t yellow = {2, 0x3F, 0x3F, 0x00, 1};
  led_cmd_t green = {3, 0x00, 0x3F, 0x00, 1};
  led_cmd_t blue = {0, 0x00, 0x00, 0x3F, 1};
  led_cmd_t violet = {1, 0x1F, 0x00, 0x3F, 1};
  led_cmd_t pink = {2, 0x3F, 0x1A, 0x3A, 1};
  led_cmd_t magenta = {3, 0x3F, 0x0C, 0x3F, 1};

  int delay = 100000;
  for (int i = 0; i < 4; i++) {
    set_led_state_handler(&red);
    nrf_delay_us(delay);
    set_led_state_handler(&orange);
    nrf_delay_us(delay);
    set_led_state_handler(&yellow);
    nrf_delay_us(delay);
    set_led_state_handler(&green);
    nrf_delay_us(delay);
    set_led_state_handler(&blue);
    nrf_delay_us(delay);
    set_led_state_handler(&violet);
    nrf_delay_us(delay);
    set_led_state_handler(&pink);
    nrf_delay_us(delay);
    set_led_state_handler(&magenta);
    nrf_delay_us(delay);
  }
  clear_led();
  disable_led();
}

void led_indicate_charging_state(bool is_charging) {
  enable_led(); // turn on, don't turn off. will be turned off after charging is
                // done.
  led_cmd_t red_on = {0, 0x3F, 0x00, 0x00, 1};
  led_cmd_t red_off = {0, 0x00, 0x00, 0x00, 1};
  led_cmd_t green_on = {3, 0x00, 0x3F, 0x00, 1};
  led_cmd_t green_off = {3, 0x00, 0x00, 0x00, 1};
  if (is_charging) {
    set_led_state_handler(&red_on);
    set_led_state_handler(&green_off);
  } else {
    set_led_state_handler(&green_on);
    set_led_state_handler(&red_off);
  }
}

void show_all(uint8_t color_index) {
  led_cmd_t led1 = {3, 0x00, 0x00, 0x00, 0};
  led_cmd_t led2 = {1, 0x00, 0x00, 0x00, 0};
  led_cmd_t led3 = {2, 0x00, 0x00, 0x00, 0};
  led_cmd_t led4 = {0, 0x00, 0x00, 0x00, 0};

  rgb_t c = colors_array[color_index].color;

  led1.r = c.r;
  led1.g = c.g;
  led1.b = c.b;
  led2.r = c.r;
  led2.g = c.g;
  led2.b = c.b;
  led3.r = c.r;
  led3.g = c.g;
  led3.b = c.b;
  led4.r = c.r;
  led4.g = c.g;
  led4.b = c.b;
  set_led_state_handler(&led1);
  set_led_state_handler(&led2);
  set_led_state_handler(&led3);
  set_led_state_handler(&led4);
}

void signal_friend_success(uint8_t color_index) {
	enable_led();
  clear_led();
  show_all(color_index);
  nrf_delay_ms(800);
  clear_led();
  nrf_delay_ms(800);
  show_all(color_index);
  nrf_delay_ms(800);
  clear_led();
  nrf_delay_ms(800);
  disable_led();
}
