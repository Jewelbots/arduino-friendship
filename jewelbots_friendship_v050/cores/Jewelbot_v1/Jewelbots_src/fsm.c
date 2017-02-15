#include <stdbool.h>
#include <stdint.h>

#include "app_timer.h"
#include "app_util.h"
#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "fds.h"
#include "mem_manager.h"
#include "nrf_assert.h"
#include "nrf_delay.h"
#include "nrf_soc.h"
#include "sdk_config.h"

#include "advertising.h"
#include "ble_central_event_handler.h"
#include "cbuf.h"
#include "dfu_handler.h"
#include "friend_data_storage.h"
#include "fsm.h"
#include "haptics_driver.h"
#include "jewelbot_service.h"
#include "jewelbot_types.h"
#include "led_driver.h"
#include "led_sequence.h"
#include "messaging.h"
#include "pmic_driver.h"
#include "power_management.h"
#include "scan.h"
#include "utils.h"


#define FRIEND_ENTER_TIMEOUT APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)
#define LED_TIMER_DELAY                                                        \
  APP_TIMER_TICKS(2000, APP_TIMER_PRESCALER) // todo: reduce to one timer. used
                                             // for adding friends.
#define SHORT_LED_TIMER_DELAY                                                  \
  APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER) // todo: reduce to one timer. used
                                             // for adding friends.

#define LED_FLASH_TIMEOUT APP_TIMER_TICKS(300, APP_TIMER_PRESCALER)
#define DECLINE_FRIEND_TIMEOUT APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER)
#define MSG_COMPOSE_TIMEOUT APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)
#define DISPLAY_MSG_TIMEOUT APP_TIMER_TICKS(7500, APP_TIMER_PRESCALER)
#define SENTINEL_VALUE 0xEE
#define USE_ALL_COLORS -1

#define fsm_ctor(me_, init_) ((me_)->state__ = (state)(init_))
#define fsm_init(me_, e_) (*(me_)->state__)((me_), (e_))
#define fsm_dispatch(me_, e_) (*(me_)->state__)((me_), (e_))
#define fsm_transition(me_, targ_) ((me_)->state__ = (state)(targ_))

static jewelbot_t jwb;
static jwb_event jwb_e;
static bool leds_on = false;
static const color_friends_t empty_friend;
static bool friend_timer_started = false;
static bool short_led_timer_delay_timer_started = false;
static buf_array_t msg_recipients;
static int32_t all_colors = 0;
static bool msg_choose_timer_started = false;
static bool msg_compose_timer_started = false;
static bool m_create_friends_list = false;
static bool friend_selected = false;
static bool already_on = false;

// valid states
static void jwb_ctor(jewelbot_t *me);
static void jwb_accept_friend(jewelbot_t *me, jwb_event const *e);
static void jwb_initial(jewelbot_t *me, jwb_event const *e);
static void jwb_add_friend(jewelbot_t *me, jwb_event const *e);
static void jwb_decline_friend(jewelbot_t *me, jwb_event const *e);

static void jwb_on(jewelbot_t *me, jwb_event const *e);
//static void jwb_off(jewelbot_t *me, jwb_event const *e);
static void jwb_charging(jewelbot_t *me, jwb_event const *e);

__attribute__((weak)) void button_press(void)
{
}


CIRCBUF_DEF(current_colors_cb, 5);

APP_TIMER_DEF(friend_cycle_timer_id);
APP_TIMER_DEF(led_timer_delay_id);
APP_TIMER_DEF(short_led_timer_delay_id);
APP_TIMER_DEF(decline_friend_id);
APP_TIMER_DEF(msg_compose_timer_id);
APP_TIMER_DEF(led_flash_timer_id);
APP_TIMER_DEF(display_msg_timer_id);

CIRCBUF_DEF(cb, 16); // move to a much higher number for testing 17?

void set_create_friends_list(void) { m_create_friends_list = true; }
void set_friend_selected(void) { friend_selected = true; }

friends_list_t * get_current_friends_list() {
	return(&jwb.friends_list);
}

void set_current_friends_list(friends_list_t *friends_list) {
	memcpy(&jwb.friends_list, friends_list, sizeof(friends_list_t));
}

static bool has_friends() {
  return (jwb.friends_list.num_of_friends != SENTINEL_VALUE &&
          jwb.friends_list.num_of_friends > 0 &&
          jwb.friends_list.num_of_friends <= MAX_NUM_OF_FRIENDS);
}
//static bool created_friends_list_in_flash() {
//  return jwb.friends_list.num_of_friends != SENTINEL_VALUE;
//}
struct jewelbot_event {
  jwb_event super_;
  uint16_t tick;
};
void jwb_ctor(jewelbot_t *me) { fsm_ctor(&me->super_, &jwb_initial); }

void jwb_initial(jewelbot_t *me, jwb_event const *e) {
  set_jwb_event_signal(JEWELBOT_ON_SIG);
  fsm_transition((fsm *)me, &jwb_on);
}
void jwb_accept_friend(jewelbot_t *me, jwb_event const *e) {
  switch (e->sig) {
	case DECLINE_FRIEND_SIG: // moving out of range, or doing nothing
  case MOVE_OUT_OF_RANGE_SIG: // radio issues disconnect.
    me->cycle = false;
    fsm_transition((fsm *)me, &jwb_decline_friend);
    break;
	default:
		if (get_ff_cent_complete() || get_ff_periph_complete()) {
			NRF_LOG_DEBUG("inside accept friend\r\n");
			app_timer_stop(decline_friend_id);
			clear_led();
			friend_selected = false;
			set_ff_cent();
			set_ff_periph();
			set_ff_index(FF_SENTINEL);
			add_or_update_friends_list();
			set_broadcast_tx_power();
			signal_friend_success(jwb.color);
			set_jwb_event_signal(JEWELBOT_ON_SIG);
			fsm_transition((fsm *)me, &jwb_on);
		}
		break;
	}
}

void jwb_decline_friend(jewelbot_t *me, jwb_event const *e) {
  switch (e->sig) {
  case DECLINE_FRIEND_SIG:
  case MOVE_OUT_OF_RANGE_SIG:
		friend_selected = false;
		set_ff_cent();
		set_ff_periph();
		set_ff_conn_cent();
		set_ff_conn_periph();
		set_ff_index(FF_SENTINEL);
    app_timer_stop(decline_friend_id);
		led_indicate_error();
    //disconnect();
		set_broadcast_tx_power();
    set_jwb_event_signal(JEWELBOT_ON_SIG);
    start_scanning();
    start_advertising(ADV_MODE_NONCONNECTABLE);
    fsm_transition((fsm *)me, &jwb_on);
    break;
  default:
    set_jwb_event_signal(JEWELBOT_ON_SIG);
    break;
  }
}
void set_pending_friend(const ble_gap_addr_t *address) {
  memcpy(&jwb.pending_friend, address, sizeof(ble_gap_addr_t));
	app_timer_start(decline_friend_id, DECLINE_FRIEND_TIMEOUT, NULL);
	disconnect();
}
ble_gap_addr_t *get_pending_friend() { return &jwb.pending_friend; }

bool valid_pending_friend() {
  uint8_t fake_address[] = {0, 0, 0, 0, 0, 0};
  if (memcmp(get_pending_friend()->addr, fake_address, sizeof(uint8_t[6])) ==
      0) {
    return false;
  }
  return true;
}
uint8_t get_num_of_friends() { return jwb.friends_list.num_of_friends; }

static bool in_friends_list(color_friends_t *friend_to_find,
                            int16_t *index_of_friend_found) {
  for (uint8_t i = 0; i < jwb.friends_list.num_of_friends; i++) {
    if (memcmp(jwb.friends_list.friends[i].address.addr,
               friend_to_find->address.addr, sizeof(uint8_t[6])) == 0) {
      *index_of_friend_found = i;
      return true;
    }
  }
  return false;
}

void add_or_update_friends_list() {
  if (jwb.friends_list.num_of_friends == MAX_NUM_OF_FRIENDS) {
    return; // todo: figure out how to handle max friends
  }
  color_friends_t _friend;
  memcpy(&_friend.address, &jwb.pending_friend, sizeof(ble_gap_addr_t));
  _friend.color = jwb.color;
  int16_t index_of_existing_friend = -1;
  if (in_friends_list(&_friend, &index_of_existing_friend)) {
    ASSERT(index_of_existing_friend != -1);
    jwb.friends_list.friends[index_of_existing_friend].color = _friend.color;
		// need to reset the messaging buffer here
		remove_from_msg_buffer();
  } else {
    memcpy(&(jwb.friends_list.friends[jwb.friends_list.num_of_friends]),
           &_friend, sizeof(color_friends_t));
    jwb.friends_list.num_of_friends = jwb.friends_list.num_of_friends + 1;
  }
  update_friends_list(&jwb.friends_list);
  save_friends(&jwb.friends_list);

}

void clear_pending_friend() {
  memcpy(&jwb.pending_friend, &empty_friend.address, sizeof(ble_gap_addr_t));
}
static void jwb_connectable_state(jewelbot_t *me, jwb_event const *e) {
  switch (e->sig) {
  case CONNECTEE_FOUND_SIG:
		if (get_ff_conn_as_central()) {
			set_jwb_event_signal(ADD_FRIEND_SIG);
			fsm_transition((fsm *)me, &jwb_add_friend);
		} else if (get_ff_conn_as_periph()) {
			flash_white();
			if ((!friend_selected) && (!already_on)) {
				flash_white();
				already_on = true;
				app_timer_stop(short_led_timer_delay_id);
				short_led_timer_delay_timer_started = false;
				leds_on = false;
			} else if (friend_selected) {
				already_on = false;
				friend_selected = false;
				set_jwb_event_signal(ACCEPT_FRIEND_SIG);
				fsm_transition((fsm *)me, &jwb_add_friend);
			}
		}
		break;
	case DECLINE_FRIEND_SIG: // moving out of range, or doing nothing
    fsm_transition((fsm *)me, &jwb_decline_friend);
    break;
  case MOVE_OUT_OF_RANGE_SIG: // radio issues disconnect.
    fsm_transition((fsm *)me, &jwb_decline_friend);
    break;
  case JEWELBOT_ON_SIG:
		set_broadcast_tx_power();
    clear_led();
    disable_led();
    fsm_transition((fsm *)me, &jwb_on);
    break;
  default:
    if (!leds_on) {
      flash_white();
    } else {
      clear_led();
      disable_led();
    }

    if (!short_led_timer_delay_timer_started) {
      uint32_t err_code = app_timer_start(short_led_timer_delay_id,
                                          SHORT_LED_TIMER_DELAY, NULL);
      APP_ERROR_CHECK(err_code);
      short_led_timer_delay_timer_started = true;
    }
    break;
  }
}

void jwb_add_friend(jewelbot_t *me, jwb_event const *e) {

  switch (e->sig) {
  case ACCEPT_FRIEND_SIG: { // button press
    me->cycle = false;
		if (get_ff_conn_as_central()) {
			clear_led();
		}


		if (!valid_pending_friend()) {
			set_jwb_event_signal(MOVE_OUT_OF_RANGE_SIG);
			fsm_transition((fsm *)me, &jwb_decline_friend);
		}
		sd_ble_gap_connect_cancel();
		start_scanning();
		set_ff_index(jwb.color);
		start_advertising(ADV_MODE_FF_NONCONN);
		fsm_transition((fsm *)me, &jwb_accept_friend);
    break;
  }
  case DECLINE_FRIEND_SIG: // moving out of range, or doing nothing
    me->cycle = false;
    fsm_transition((fsm *)me, &jwb_decline_friend);
    break;
  case MOVE_OUT_OF_RANGE_SIG: // radio issues disconnect.
    me->cycle = false;
    fsm_transition((fsm *)me, &jwb_decline_friend);
    break;
  case ADD_FRIEND_SIG:
    if (!me->cycle) {
      me->current_cycle = 0;
      me->cycle = true;
    }
    cycle_through_all_colors(me);
    all_colors = USE_ALL_COLORS;
    app_timer_start(led_timer_delay_id, LED_TIMER_DELAY, (void *)&all_colors);

    break;
  default:
    if (e->sig == CONNECTEE_FOUND_SIG) {
      set_jwb_event_signal(ADD_FRIEND_SIG);
    }
    if (!valid_pending_friend()) {
      set_jwb_event_signal(MOVE_OUT_OF_RANGE_SIG);
    }
    break;
  }
}
void reset_msg_compose_timer() {
  msg_compose_timer_started = false;
  app_timer_stop(msg_compose_timer_id);
  app_timer_start(msg_compose_timer_id, MSG_COMPOSE_TIMEOUT, NULL);
  msg_compose_timer_started = true;
}

static void jwb_compose_message(jewelbot_t *me, jwb_event const *e) {
  switch (e->sig) {
  case MSG_FINISHED_SIG:
    set_jwb_event_signal(JEWELBOT_ON_SIG); // check in on state to send messages
    start_advertising(ADV_MODE_MSG_NONCONN);
		app_timer_start(display_msg_timer_id, DISPLAY_MSG_TIMEOUT, NULL);
    fsm_transition((fsm *)me, &jwb_on);
    break;
  case MSG_COLOR_SELECTED_SIG:
  default:
    if (!msg_compose_timer_started) {
      app_timer_start(msg_compose_timer_id, MSG_COMPOSE_TIMEOUT, NULL);
      msg_compose_timer_started = true;
    }
    break;
  }
}

static void jwb_message_received(jewelbot_t *me, jwb_event const *e) {
  switch (e->sig) {
  case MSG_RECEIVED_SIG:
    if (has_message_to_play()) {
      NRF_LOG_DEBUG("PLAYING MESSAGE!\r\n");
      play_message();
    } else {
      fsm_transition((fsm *)me, &jwb_on);
    }
    break;
  case MSG_PLAYED_SIG:
    set_jwb_event_signal(JEWELBOT_ON_SIG);
    //disconnect();
    fsm_transition((fsm *)me, &jwb_on);
    break;
  }
}

static void jwb_messaging(jewelbot_t *me, jwb_event const *e) {
  switch (e->sig) {
  case MSG_MODE_SIG:
    if (!msg_choose_timer_started) {
      get_elements_in_buffer(&current_colors_cb, &msg_recipients);
      if (msg_recipients.buf_length > 0) {
				NRF_LOG_PRINTF_DEBUG("msg recipients = %u\r\n", msg_recipients.buf_length);
        all_colors = msg_recipients.buf_length;
        app_timer_start(led_timer_delay_id, LED_TIMER_DELAY, NULL);
        msg_choose_timer_started = true;
        cycle_through_present_colors(&msg_recipients);
      } else {
        NRF_LOG_DEBUG("NO RECIPIENTS\r\n");
        set_jwb_event_signal(JEWELBOT_ON_SIG);
        if (!get_arduino_coding()){
				    led_cmd_t red_on = {0, 0x3F, 0x00, 0x00, 1};
				    set_led_state_handler(&red_on);
				    uint32_t err_code = app_timer_start(led_flash_timer_id,
                                LED_FLASH_TIMEOUT, NULL);
            APP_ERROR_CHECK(err_code);
        } else {
          button_press();
        }
      }
    }
    break;
  case MSG_COLOR_SELECTED_SIG:
    reset_current_msg_iterator();
    fsm_transition((fsm *)me, &jwb_compose_message);
    break;
  case JEWELBOT_ON_SIG:
    fsm_transition((fsm *)me, &jwb_on);
    break;
  default:
    break;
  }
}

static void jwb_pair_with_master(jewelbot_t *me, jwb_event const *e) {
  switch (e->sig) {
    case BONDED_TO_MASTER_DEVICE_SIG:
      set_jwb_event_signal(JEWELBOT_ON_SIG);
      break;
    case JEWELBOT_ON_SIG:
			set_broadcast_tx_power();
      start_advertising(ADV_MODE_NONCONNECTABLE);
      fsm_transition((fsm *)me, &jwb_on);
      break;
    case JEWELBOT_CONNECTED_TO_MASTER_SIG:
      if (is_connected()) {
        led_indicate_connected();
      }
      else {
				set_broadcast_tx_power();
        set_jwb_event_signal(JEWELBOT_ON_SIG);
        fsm_transition((fsm *)me, &jwb_on);
      }
      break;
   default:
    if (!is_connected()) {
      if (!leds_on) {
        led_indicate_bonding_state();
      } else {
        clear_led();
        disable_led();
      }
      if (!short_led_timer_delay_timer_started) {
        uint32_t err_code = app_timer_start(short_led_timer_delay_id,
                                            SHORT_LED_TIMER_DELAY, NULL);
        APP_ERROR_CHECK(err_code);
        short_led_timer_delay_timer_started = true;
      }
    }
    else {
      set_jwb_event_signal(JEWELBOT_CONNECTED_TO_MASTER_SIG);
    }
    break;
  }
}

void jwb_charging(jewelbot_t *me, jwb_event const *e) {
  switch (e->sig) {
  case DFU_UPDATE_SIG:
    bootloader_start();
    break;
  case JEWELBOT_ON_SIG:
    fsm_transition((fsm *)me, &jwb_on);
    break;
  case BOND_WITH_MASTER_DEVICE_SIG:
    set_friend_finding_tx_power();
    start_advertising(ADV_MODE_CONNECTABLE);
    fsm_transition((fsm *)me, &jwb_pair_with_master);
    break;
  default:
    if (!pmic_5V_present()) {
			clear_led();
			disable_led();
      set_jwb_event_signal(JEWELBOT_ON_SIG);
    } else {
      led_indicate_charging_state(pmic_is_charging());
    }
    break; // wait
  }
}

static void push_current_color_onto_msg_buffer(uint8_t found_friend_idx) {
  c_buf_code_t err_code = c_buf_push(
      &current_colors_cb, jwb.friends_list.friends[found_friend_idx].color);
  if (err_code != BUF_SUCCESS) {
  }
}
static void check_for_friends() {
  uint8_t found_friend_idx;
  c_buf_code_t buf_err_code = c_buf_pop(&cb, &found_friend_idx);
  if (buf_err_code == BUF_SUCCESS && found_friend_idx != SENTINEL_VALUE) {
    if (found_friend_idx > MAX_NUM_OF_FRIENDS) {
      return;
    } else {
      if (has_friends()) {
        led_sequence_buffer_push(
            jwb.friends_list.friends[found_friend_idx].color);
				//NRF_LOG_PRINTF_DEBUG("push current color, index: %u\r\n", found_friend_idx);
        push_current_color_onto_msg_buffer(found_friend_idx);
      }
    }
  }
}

uint32_t get_color_index_for_friend(uint8_t friend_index,
                                    uint8_t *color_index) {
  if (friend_index > MAX_NUM_OF_FRIENDS) {
    return NRF_ERROR_NOT_FOUND;
  }
  *color_index = jwb.friends_list.friends[friend_index].color;
  return NRF_SUCCESS;
}



void jwb_on(jewelbot_t *me, jwb_event const *e) {
	uint32_t err_code;
  switch (e->sig) {

  case MSG_RECEIVED_SIG:
    fsm_transition((fsm *)me, &jwb_message_received);
    break;
  case BUTTON_PRESS_SIG:
  case MSG_MODE_SIG:
		err_code = app_timer_stop(friend_cycle_timer_id);
    friend_timer_started = false;
		APP_ERROR_CHECK(err_code);
    fsm_transition((fsm *)me, &jwb_messaging);
    break;
  case BOND_WITH_MASTER_DEVICE_SIG:
    fsm_transition((fsm *)me, &jwb_pair_with_master);
    break;
  case CHARGING_SIG:
		clear_led();
    fsm_transition((fsm *)me, &jwb_charging);
    break;
  case BUTTON_PRESS_LONG_SIG:
		err_code = app_timer_stop(friend_cycle_timer_id);
    friend_timer_started = false;
		APP_ERROR_CHECK(err_code);
    set_jwb_event_signal(WAITING_FOR_CONNECTEE_SIG);
#ifndef DEV_BOARDS
    set_friend_finding_tx_power();
#endif
    start_advertising(ADV_MODE_CONNECTABLE);
    fsm_transition((fsm *)me, &jwb_connectable_state);
    break;
  case POWER_DOWN_SIG:
//    fsm_transition((fsm *)me, &jwb_off);
		// Move power down up here to remove one loop through main and the state machine = faster signal to user
		signal_power_down();
    sd_power_system_off();
    break;
  case JEWELBOT_ON_SIG:
    check_for_friends(); // push happens faster than showing
    if (!friend_timer_started) {
			//NRF_LOG_DEBUG("in friend timer started\r\n");
      enable_led();
      uint32_t err_code =
          app_timer_start(friend_cycle_timer_id, FRIEND_ENTER_TIMEOUT,
                          (void *)NULL); // todo: Check if we can just start the
                                         // timer again with different timeout
                                         // for breathing
      APP_ERROR_CHECK(err_code);
      friend_timer_started = true;

      if (has_friends()) {
				//NRF_LOG_DEBUG("Start Breathe\r\n");
        breathe_friends(); // pops and shows
#ifdef DEBUG
        if (is_advertising()) {
          NRF_LOG_DEBUG("@");
        }
        if (is_scanning()) {
          NRF_LOG_DEBUG("$");
        }
#endif
      }
    }
    if (pmic_5V_present()) {

      set_jwb_event_signal(CHARGING_SIG);

    }

    break;
  default:
    break;
  }
}


void push_current(const ble_gap_addr_t *peer_addr, uint8_t *index) {
  c_buf_code_t err_code = c_buf_push(&cb, *index);
  UNUSED_PARAMETER(err_code);
}

void set_jwb_found(const ble_gap_addr_t *peer_addr, uint8_t adv_type, bool dest_ff_mode) {
  uint8_t sig = get_current_event()->sig;
  if (sig == JEWELBOT_ON_SIG) {
    uint8_t index = 0;
    if (jwb_friend_found(peer_addr, &index)) {
      push_current(peer_addr, &index);
    }
  }
}
void set_jwb_event_signal(enum jwb_signal_t signal) { jwb_e.sig = signal; }
bool jwb_friend_found(const ble_gap_addr_t *peer_addr, uint8_t *index) {
  if (!has_friends()) {
    return false;
  }
  uint8_t sizeOfFriends =
      sizeof(jwb.friends_list.friends) / sizeof(color_friends_t);
  for (uint8_t i = 0; i < sizeOfFriends; i++) {
    if (memcmp(jwb.friends_list.friends[i].address.addr, peer_addr->addr,
               (sizeof(peer_addr->addr) / sizeof(peer_addr->addr[0]))) == 0) {
      *index = i;
      return true;
    }
  }
  return false;
}

bool jwb_pending_friend_found(const ble_gap_addr_t *peer_addr) {
    if (memcmp(jwb.pending_friend.addr, peer_addr->addr,
               (sizeof(peer_addr->addr) / sizeof(peer_addr->addr[0]))) == 0) {
      return true;
    }
  return false;
}

void remove_from_msg_buffer() {
  uint8_t val;
  while (c_buf_pop(&current_colors_cb, &val) == BUF_SUCCESS) {
    // just clearing buffer
  }
}

static void friend_cycle_timeout_handler(void *p_context) {

  UNUSED_PARAMETER(p_context);
  uint32_t err_code;
  err_code = app_timer_stop(friend_cycle_timer_id);
  clear_led();
  APP_ERROR_CHECK(err_code);
  if (err_code == NRF_SUCCESS) {
    friend_timer_started = false;
  }
}

static void msg_compose_timeout_handler(void *p_context) {
  UNUSED_PARAMETER(p_context);
  app_timer_stop(msg_compose_timer_id);
  msg_compose_timer_started = false;
	clear_led();
  set_jwb_event_signal(MSG_FINISHED_SIG);
  NRF_LOG_PRINTF_DEBUG("\r\nMSG_FINISHED_SIG: %u\r\n", get_current_event()->sig);
}
static void led_timer_delay_handler(void *p_context) {
  UNUSED_PARAMETER(p_context);
  uint32_t err_code;
  err_code = app_timer_stop(led_timer_delay_id);
  msg_choose_timer_started = false;
  APP_ERROR_CHECK(err_code);
  if (all_colors == USE_ALL_COLORS) {
    if (jwb.current_cycle >= 4) {
      set_current_cycle(0);
    } else {
      set_current_cycle(jwb.current_cycle + 1);
    }
  }
}
static void short_led_timer_delay_handler(void *p_context) {
  uint32_t err_code;
  UNUSED_PARAMETER(p_context);
  err_code = app_timer_stop(short_led_timer_delay_id);
  short_led_timer_delay_timer_started = false;
  APP_ERROR_CHECK(err_code);
  leds_on = !leds_on;
};

static void led_flash_timeout_handler(void *p_context) {
	uint32_t err_code;
  UNUSED_PARAMETER(p_context);
  err_code = app_timer_stop(led_flash_timer_id);
	APP_ERROR_CHECK(err_code);
	clear_led();
}

static void display_msg_timeout_handler(void *p_context) {
	uint32_t err_code;
  UNUSED_PARAMETER(p_context);
  err_code = app_timer_stop(display_msg_timer_id);
	APP_ERROR_CHECK(err_code);
	start_advertising(ADV_MODE_NONCONNECTABLE);
}
static void decline_friend_handler(void *p_context) {
  UNUSED_PARAMETER(p_context);
  signal sig = get_current_event()->sig;

  if ((sig == WAITING_FOR_CONNECTEE_SIG) ||
			(sig == CONNECTEE_FOUND_SIG) ||
			(sig == ADD_FRIEND_SIG) ||
			(sig == ACCEPT_FRIEND_SIG) ||
			(sig == BUTTON_PRESS_LONG_SIG)) {
    set_jwb_event_signal(DECLINE_FRIEND_SIG);
  }
}
static void initialize_current_colors_buffer(void) {
  uint8_t data;
  memset(&msg_recipients, 0, sizeof(buf_array_t));
  for (uint8_t i = 0; i < 5; i++) {
    c_buf_init(&current_colors_cb, SENTINEL_VALUE);
    c_buf_pop(&current_colors_cb, &data);
  }
}

static void initialize_friend_buffer(void) {
  uint8_t data;
  for (uint8_t i = 0; i < MAX_NUM_OF_FRIENDS; i++) {
    c_buf_init(&cb, SENTINEL_VALUE);
    c_buf_pop(&cb, &data);
  }
}

void initialize_friends_list(void) {
	//NRF_LOG_DEBUG("initiliaze friends list\r\n");
  memset(&jwb.friends_list, 0, sizeof(friends_list_t));
  load_friends(&jwb.friends_list);
  if (m_create_friends_list) {
		//NRF_LOG_DEBUG("create friends list\r\n");
		m_create_friends_list = false;
    create_friends_list_in_flash(&jwb.friends_list);
  } else {
		//reset_flash_op_status();
		start_advertising(ADV_MODE_NONCONNECTABLE);
		start_scanning();
	}
  set_friends_list_loaded();
}
void jewelbot_state_machine_init(void) {
  uint32_t err_code;

  jwb_ctor(&jwb);
  fsm_init((fsm *)&jwb, 0);

  err_code =
      app_timer_create(&friend_cycle_timer_id, APP_TIMER_MODE_SINGLE_SHOT,
                       friend_cycle_timeout_handler);
  APP_ERROR_CHECK(err_code);
  err_code = app_timer_create(&led_timer_delay_id, APP_TIMER_MODE_REPEATED,
                              led_timer_delay_handler);
  APP_ERROR_CHECK(err_code);
  err_code =
      app_timer_create(&short_led_timer_delay_id, APP_TIMER_MODE_REPEATED,
                       short_led_timer_delay_handler);
  APP_ERROR_CHECK(err_code);
  err_code = app_timer_create(&decline_friend_id, APP_TIMER_MODE_SINGLE_SHOT,
                              decline_friend_handler);
  err_code = app_timer_create(&msg_compose_timer_id, APP_TIMER_MODE_SINGLE_SHOT,
                              msg_compose_timeout_handler);
  APP_ERROR_CHECK(err_code);
	err_code = app_timer_create(&led_flash_timer_id, APP_TIMER_MODE_SINGLE_SHOT, led_flash_timeout_handler);
	err_code = app_timer_create(&display_msg_timer_id, APP_TIMER_MODE_SINGLE_SHOT, display_msg_timeout_handler);
  initialize_led_buffer();
  initialize_current_colors_buffer();
  initialize_friend_buffer();
  dfu_handler_init();
}
void state_machine_dispatch(void) {
  fsm_dispatch((fsm *)&jwb, (jwb_event *)&jwb_e);
}
jwb_event *get_current_event() { return &jwb_e; }

void set_current_color(uint8_t color_index) { jwb.color = color_index; }
void set_current_cycle(uint8_t cycle) { jwb.current_cycle = cycle; }
uint8_t get_current_cycle(void) { return jwb.current_cycle; }

bool friend_adding_mode(void) {
  jwb_event *evt = get_current_event();
  return (evt->sig == CONNECTEE_FOUND_SIG ||
          evt->sig == WAITING_FOR_CONNECTEE_SIG ||
          evt->sig == BUTTON_PRESS_LONG_SIG ||
					evt->sig == ADD_FRIEND_SIG ||
					evt->sig == ACCEPT_FRIEND_SIG);
}

bool dest_friend_adding_mode(data_t *manufacturer_data) {

  if (manufacturer_data->data_len < 2) {
    return false;
  }
  if (manufacturer_data->p_data[0] == 0x03 &&
      manufacturer_data->p_data[1] == 0x21) { // we are a jewelbot.
    if (manufacturer_data->p_data[2] == 0x10) { // 0x10 : pair bit
      return true;
    }
  }
  return false;
}

bool dest_friend_sharing_mode(data_t *manufacturer_data) {

  if (manufacturer_data->data_len < 2) {
    return false;
  }
  if (manufacturer_data->p_data[0] == 0x03 &&
      manufacturer_data->p_data[1] == 0x21) { // we are a jewelbot.
		NRF_LOG_PRINTF_DEBUG("adv data = %x\r\n", manufacturer_data->p_data[2]);
    if (manufacturer_data->p_data[2] == 0xCE) {
      return true;
    }
  }
  return false;
}

bool dest_jewelbot(data_t *manufacturer_data) {

  if (manufacturer_data->data_len < 2) {
    return false;
  }
  if (manufacturer_data->p_data[0] == 0x03 &&
      manufacturer_data->p_data[1] == 0x21) { // we are a jewelbot.
    return true;
  }
  return false;
}

void print_friends_list() {
  NRF_LOG_DEBUG("------- BEGIN FRIENDS LIST -------\r\n");

  NRF_LOG_PRINTF_DEBUG("number of friends: %u\r\n",
                    jwb.friends_list.num_of_friends);

  for (uint8_t i = 0; i < MAX_NUM_OF_FRIENDS; i++) { //jwb.friends_list.num_of_friends; i++) {
    uint8_t str[50] = {0};
    ble_address_to_string_convert(jwb.friends_list.friends[i].address, str);
    NRF_LOG_PRINTF_DEBUG("%u, ADDR:%s, COLOR: %u\r\n", i + 1, str,
                      jwb.friends_list.friends[i].color);
  }
  NRF_LOG_DEBUG("------- END FRIENDS LIST --------\r\n");
}
