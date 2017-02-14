#ifndef FSM_H_
#define FSM_H_
#include "advertising.h"
#include "cbuf.h"
#include "jewelbot_types.h"
#include <stdbool.h>
#include <stdint.h>

enum jwb_signal_t {
  BUTTON_OK_SIG, // 0
  BUTTON_PRESS_SIG,
  BUTTON_DOUBLE_TAP_SIG,
  BUTTON_PRESS_LONG_SIG, // 3
  POWER_DOWN_SIG,
  ADD_FRIEND_SIG,
  CHARGING_SIG, // 6
  ACCEPT_FRIEND_SIG,
  DECLINE_FRIEND_SIG,
  MOVE_OUT_OF_RANGE_SIG, // 9
  WAITING_FOR_CONNECTEE_SIG,
  CONNECTEE_FOUND_SIG,
  JEWELBOT_ON_SIG, // 12
  DFU_UPDATE_SIG,
  MSG_MODE_SIG,           // 14
  MSG_COLOR_SELECTED_SIG, // 15
  MSG_FINISHED_SIG,
  MSG_SEND_SIG,
  MSG_RECEIVED_SIG,
  MSG_PLAYED_SIG, // 18
  FLASH_OP_COMPLETE_SIG,
  FLASH_OP_GC_COMPLETE_SIG,
  BOND_WITH_MASTER_DEVICE_SIG, //21
  BONDED_TO_MASTER_DEVICE_SIG,
  JEWELBOT_CONNECTED_TO_MASTER_SIG

};

typedef short signal;
typedef struct jwb_event jwb_event;
typedef struct fsm fsm;
typedef struct jewelbot_t jewelbot_t;
typedef void (*state)(fsm *, jwb_event const *);
struct fsm {
  state state__;
};

struct jewelbot_t {
  fsm super_;
  friends_list_t friends_list;
  cbuf_t current;
  uint8_t color; // adding friend
  bool volatile cycle;
  uint8_t current_cycle;
  ble_gap_addr_t pending_friend;
};

struct jwb_event {
  signal volatile sig;
  uint16_t volatile ticks;
};

void button_press(void);

void set_create_friends_list(void);
void jewelbot_state_machine_init(void);
void state_machine_dispatch(void);

jwb_event *get_current_event(void);

void add_or_update_friends_list(void);

bool jwb_friend_found(ble_gap_addr_t const *peer_addr, uint8_t *index);
bool jwb_pending_friend_found(ble_gap_addr_t const *peer_addr);

void reset_msg_compose_timer(void);
void push_current(const ble_gap_addr_t *peer_addr, uint8_t *index);
void set_jwb_found(ble_gap_addr_t const *peer_addr, uint8_t adv_type, bool dest_ff_mode);
void set_current_color(uint8_t color_index);
void set_current_cycle(uint8_t cycle);
void set_jwb_event_signal(enum jwb_signal_t signal);
uint8_t get_current_cycle(void);
bool friend_adding_mode(void);
bool dest_friend_adding_mode(data_t *manufacturer_data);
bool dest_friend_sharing_mode(data_t *manufacturer_data);
bool dest_jewelbot(data_t *manufacturer_data);
void set_pending_friend(const ble_gap_addr_t *address);
bool valid_pending_friend(void);
void initialize_friends_list(void);
void clear_pending_friend(void);
void remove_from_msg_buffer(void);
uint8_t get_num_of_friends(void);
ble_gap_addr_t *get_pending_friend(void);
uint32_t get_color_index_for_friend(uint8_t friend_index, uint8_t *color_index);
void print_friends_list(void);
friends_list_t * get_current_friends_list(void);
void set_current_friends_list(friends_list_t *friends_list);
void set_friend_selected(void);

#endif
