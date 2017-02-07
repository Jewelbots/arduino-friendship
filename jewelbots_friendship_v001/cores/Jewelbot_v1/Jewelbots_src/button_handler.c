#include "app_button.h"
#include "app_timer.h"
#include "nrf_drv_gpiote.h"
#include "nrf_soc.h"
#include "sdk_config.h"

#include "ble_central_event_handler.h"
#include "bootloader_types.h"
#include "button_handler.h"
#include "fsm.h"
#include "haptics_driver.h"
#include "jewelbot_gpio.h"
#include "led_driver.h"
#include "led_sequence.h"
#include "messaging.h"
#include "pmic_driver.h"
#include "scan.h"
#include "utils.h"

#ifdef REAL_JEWELBOT
#define BUTTON_DETECTION_DELAY APP_TIMER_TICKS(50, APP_TIMER_PRESCALER)
#else
#define BUTTON_DETECTION_DELAY APP_TIMER_TICKS(50, APP_TIMER_PRESCALER)
#endif
#ifdef REAL_JEWELBOT
#define POWER_OFF_TICKS 160000
#define DOUBLE_TAP_TICKS 200
#define BUTTON_PRESS_TICKS 1500
#define MSG_SHORT_TICKS 1500
#define MSG_LONG_TICKS 12000
#define BUTTON_PRESS_LONG_TICKS 65000
#else
#define POWER_OFF_TICKS 50000
#define DOUBLE_TAP_TICKS 65535
#define MSG_LONG_TICKS 12000
#define MSG_SHORT_TICKS 2500
#define BUTTON_PRESS_TICKS 4000

#define BUTTON_PRESS_LONG_TICKS 20000
#endif
#define TIMEOUT_TICKS (10UL)
#define BUTTON_PRESS_MSG_LONG_TICKS (12000)
static uint32_t button_tick = 0;
static uint32_t button_tick_release = 0;
static uint32_t total_ticks = 0;
static volatile uint16_t button_press_count = 0;
static volatile uint16_t button_min_hand = 0;
static volatile bool button_pressed = false;
static volatile uint8_t button_sequence_number = 0;
static volatile uint8_t last_button_sequence_number = 255;
static volatile bool power_off_signaled = false;
APP_TIMER_DEF(m_button_tick_timer);

static void button_handler(uint8_t button, uint8_t action) {
  uint32_t err_code;
  switch (action) {
  case APP_BUTTON_PUSH:
    app_timer_start(m_button_tick_timer, 65535, NULL);
    err_code = app_timer_cnt_get(&button_tick);
    APP_ERROR_CHECK(err_code);
    break;
  case APP_BUTTON_RELEASE:
    err_code = app_timer_cnt_get(&button_tick_release);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_stop(m_button_tick_timer);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_cnt_diff_compute(button_tick_release, button_tick,
                                          &total_ticks);
    APP_ERROR_CHECK(err_code);
    jwb_event *evt = get_current_event();
    NRF_LOG_PRINTF_DEBUG("UP: %u, sig: %u\r\n", total_ticks, evt->sig);

    switch (evt->sig) {
    case MSG_COLOR_SELECTED_SIG: {
      uint8_t msg_color = get_current_msg_color_cycle();
      if (total_ticks >= BUTTON_PRESS_MSG_LONG_TICKS) {
        msg_queue_push(msg_color, MSG_LONG);
      } else if (total_ticks >= MSG_SHORT_TICKS) {
        msg_queue_push(msg_color, MSG_SHORT);
      }
      reset_msg_compose_timer();
      break;
    }
    case MSG_MODE_SIG: {
      set_jwb_event_signal(MSG_COLOR_SELECTED_SIG);
      break;
    }
    case ADD_FRIEND_SIG: {
      //if (!valid_pending_friend()) {
        //set_jwb_event_signal(MOVE_OUT_OF_RANGE_SIG);
      //} else {
        set_jwb_event_signal(ACCEPT_FRIEND_SIG);
      //}
      break;
    }
    case CONNECTEE_FOUND_SIG:
			if (get_ff_conn_as_central()) {
				set_jwb_event_signal(ADD_FRIEND_SIG);
			}
      break;
    case JEWELBOT_ON_SIG:
      if (total_ticks >= POWER_OFF_TICKS) {
        set_jwb_event_signal(POWER_DOWN_SIG);
      } else if (total_ticks >= BUTTON_PRESS_LONG_TICKS) {
        set_jwb_event_signal(BUTTON_PRESS_LONG_SIG);
      } else if (total_ticks >= BUTTON_PRESS_TICKS) {
        NRF_LOG_PRINTF_DEBUG("CURRENT_SIG: %u\r\n", get_current_event()->sig);
        set_jwb_event_signal(MSG_MODE_SIG);
        NRF_LOG_DEBUG("SETTING MSG_MODE_SIG\r\n");
      }
      break;
    case WAITING_FOR_CONNECTEE_SIG:
      if (total_ticks >= BUTTON_PRESS_TICKS) {
				// Getting out of friend finding mode, need to go back to non-connectable adv
				start_advertising(ADV_MODE_NONCONNECTABLE);
        set_jwb_event_signal(JEWELBOT_ON_SIG);
      }
      break;
    case DFU_UPDATE_SIG:
      set_jwb_event_signal(JEWELBOT_ON_SIG);
      break;
    case CHARGING_SIG:
      if (total_ticks >= BUTTON_PRESS_TICKS && total_ticks < BUTTON_PRESS_LONG_TICKS) {
        set_jwb_event_signal(BOND_WITH_MASTER_DEVICE_SIG);
      }
      else if (total_ticks >= BUTTON_PRESS_LONG_TICKS) {
        if (pmic_5V_present()) {
          set_jwb_event_signal(DFU_UPDATE_SIG);
        }
      }
      break;

    default:
      if (total_ticks >= BUTTON_PRESS_TICKS) {
        set_jwb_event_signal(JEWELBOT_ON_SIG);
      }
      break;
    }

    break;
  }
}

#ifdef REAL_JEWELBOT
static app_button_cfg_t buttons[] = {
    {BUTTON_PIN, APP_BUTTON_ACTIVE_HIGH, NRF_GPIO_PIN_NOPULL, button_handler},
};
#else
static app_button_cfg_t buttons[] = {
    {BUTTON_PIN, APP_BUTTON_ACTIVE_LOW, NRF_GPIO_PIN_PULLUP, button_handler}};
#endif

static void timer_handler(void *p_context) { UNUSED_PARAMETER(p_context); }

void buttons_init(void) {
  uint32_t err_code = 0;

  err_code = app_button_init((app_button_cfg_t *)buttons,
                             sizeof(buttons) / sizeof(buttons[0]),
                             BUTTON_DETECTION_DELAY);
  APP_ERROR_CHECK(err_code);
  err_code = app_button_enable();
  APP_ERROR_CHECK(err_code);
#ifdef REAL_JEWELBOT
  nrf_gpio_cfg_sense_input(WAKEUP_BUTTON_PIN, NRF_GPIO_PIN_NOPULL,
                           NRF_GPIO_PIN_SENSE_LOW);
#endif
  err_code = app_timer_create(&m_button_tick_timer, APP_TIMER_MODE_REPEATED,
                              timer_handler);
}
