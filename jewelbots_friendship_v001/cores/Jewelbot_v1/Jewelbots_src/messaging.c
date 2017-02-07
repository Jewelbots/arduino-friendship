#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "common_defines.h"
#include "nrf_delay.h"
#include "nrf_error.h"
#include "nrf_log.h"

#include "ble_central_event_handler.h"
#include "fsm.h"
#include "haptics_driver.h"
#include "jewelbot_types.h"
#include "led_driver.h"
#include "led_sequence.h"
#include "messaging.h"
#include "utils.h"


static uint8_t msg_queue[COLORS_UNLOCKED][MAX_MSG_SIZE];
static int8_t msg_head[COLORS_UNLOCKED];
static uint8_t has_unsent_messages[COLORS_UNLOCKED];
static jwb_message_t pending_message_to_play;
static bool has_message = false;
static uint8_t pending_message[50];
static uint8_t msg_tag_queue[MSG_TAG_QUEUE_LENGTH][MSG_TAG_LENGTH];
static uint8_t msg_tag_head = 0;

void messaging_init(void) {
  memset(msg_queue, 0,
         sizeof(msg_queue[0][0]) * COLORS_UNLOCKED * MAX_MSG_SIZE);
  memset(msg_head, 0, sizeof(msg_head) / sizeof(msg_head[0]));
  memset(has_unsent_messages, 0,
         sizeof(has_unsent_messages) / sizeof(has_unsent_messages[0]));
  memset(pending_message, 0,
         sizeof(pending_message) / sizeof(pending_message[0]));
  memset(&pending_message_to_play, 0, sizeof(jwb_message_t));
	memset(msg_tag_queue, 0, sizeof(msg_tag_queue[0][0]) * MSG_TAG_QUEUE_LENGTH * MSG_TAG_LENGTH);
}

void msg_queue_push(uint8_t color_index, uint8_t msg_bit) {

  if (msg_head[color_index] > MAX_MSG_SIZE - 1) {
    return; // Can't add any more.
  }
  NRF_LOG_PRINTF_DEBUG("PUSH: msg_head: %u, color_index: %u, msg_bit: %u\r\n",
                 msg_head[color_index], color_index, msg_bit);
  msg_queue[color_index][msg_head[color_index]] = msg_bit;
  msg_head[color_index] = msg_head[color_index] + 1;
  has_unsent_messages[color_index] = 1;
}

void msg_queue_pop(uint8_t color_index, uint8_t *msg, uint8_t *size) {
  uint8_t i = 0;
  *size = msg_head[color_index];
  NRF_LOG_PRINTF_DEBUG("Size is now: %u\r\n", *size);
  while (msg_head[color_index] >= 0) {
    msg[i] = msg_queue[color_index][i];
    i++;
    if (msg_head[color_index] > 0) {
      msg_head[color_index]--;
    }
    if (msg_head[color_index] == 0) {
      return;
    }
  }
}


bool has_message_to_send(void) {
  for (uint8_t i = 0; i < COLORS_UNLOCKED; i++) {
    if (has_unsent_messages[i]) {
      return true;
    }
  }
  return false;
}
bool pending_message_for_friend_index(uint8_t friend_index) {
  uint8_t color_index = 0;
  uint32_t err_code = get_color_index_for_friend(friend_index, &color_index);

  if (err_code == NRF_SUCCESS) { // found color for friend,
    if (color_index < COLORS_UNLOCKED) {
      if (has_unsent_messages[color_index] > 0) {
        return true;
      }
      return false;
    }
    return false;
  }
  return false;
}
bool pending_message_for_friend(ble_gap_addr_t *address) {
  uint8_t index = 0;
  uint8_t color_index = 0;
  if (jwb_friend_found(address, &index)) {
    uint32_t err_code = get_color_index_for_friend(index, &color_index);
    if (err_code == NRF_SUCCESS) { // found color for friend,
      if (color_index < COLORS_UNLOCKED) {
        if (has_unsent_messages[color_index] > 0) {
          return true;
        }
        return false;
      }
      return false;
    }
    return false;
  }
  return false;
}

void queue_message_for_play(uint8_t *msg, uint8_t msg_length) {
  //NRF_LOG_DEBUG("QUEUED MSG: ");
  uint8_t j = 0;
  for (uint8_t i = MSG_START_INDEX; i < msg_length; i++) {
    pending_message_to_play.msg[j] = msg[i];
    //NRF_LOG_PRINTF_DEBUG("%u ", msg[i]);
    j++;
  }
  //NRF_LOG_DEBUG("\r\n");
  pending_message_to_play.msg_length = msg_length - 1;
  pending_message_to_play.color_index = msg[COLOR_INDEX];
  NRF_LOG_PRINTF_DEBUG("SIZE: %u, COLOR_INDEX: %u\r\n",
                 pending_message_to_play.msg_length,
                 pending_message_to_play.color_index);
  for (uint8_t i = 0; i < pending_message_to_play.msg_length; i++) {
    NRF_LOG_PRINTF_DEBUG("%u ", pending_message_to_play.msg[i]);
  }
  NRF_LOG_DEBUG("\r\n");
  has_message = true;
}

bool has_message_to_play(void) { return has_message; }
void play_message() {
  if (pending_message_to_play.msg_length >= MAX_MSG_SIZE) {
    pending_message_to_play.msg_length = MAX_MSG_SIZE;
  }
  uint32_t msg_delay = 0;
  show_all(pending_message_to_play.color_index);
  for (uint8_t i = 0; i < pending_message_to_play.msg_length; i++) {
    if (pending_message_to_play.msg[i] == MSG_SHORT) {
      haptics_msg_short();
      msg_delay = 500;
    } else if (pending_message_to_play.msg[i] == MSG_LONG) {
      haptics_msg_long();
      msg_delay = 1500;
    }
    nrf_delay_ms(msg_delay);
  }
  clear_led();
  disable_led();
  has_message = false;
  memset(&pending_message_to_play, 0, sizeof(jwb_message_t));
  set_jwb_event_signal(MSG_PLAYED_SIG);
}

void record_msg_tag(uint8_t msg_tag[MSG_TAG_LENGTH]) {
	for (uint8_t i = 0; i < MSG_TAG_LENGTH; i++) {
		msg_tag_queue[msg_tag_head][i] = msg_tag[i];
	}
	msg_tag_head++;
	if (msg_tag_head == MSG_TAG_QUEUE_LENGTH) {
		msg_tag_head = 0;
	}
}

bool msg_tag_in_queue(uint8_t msg_tag[MSG_TAG_LENGTH]) {
	uint8_t match = 0;
	uint8_t recent = 0;
	if (msg_tag_head == 0) {
		recent = MSG_TAG_QUEUE_LENGTH - 1;
	} else {
		recent = msg_tag_head - 1;
	}
	//NRF_LOG_PRINTF_DEBUG("recent = %u, head = %u\r\n", recent, msg_tag_head);
	for (uint8_t i = 0; i < MSG_TAG_QUEUE_LENGTH; i++) {	
		for (uint8_t j = 0; j < MSG_TAG_LENGTH; j++) {
			if (msg_tag_queue[recent][j] == msg_tag[j]) {
				//NRF_LOG_PRINTF_DEBUG("before: match = %u, 1 << j = %u\r\n", match, (1<<j));
				match = match | (1 << j);
				//NRF_LOG_PRINTF_DEBUG("match = %u \r\n", match);
			}		
		}
		//NRF_LOG_PRINTF_DEBUG("final match= %u, comp value = %u\r\n", match, ((1 << MSG_TAG_LENGTH) - 1));
		if (match == ((1 << MSG_TAG_LENGTH) - 1)) {
			return true;
		} else {
			match = 0;
		}
		if (recent == 0) {
			recent = MSG_TAG_QUEUE_LENGTH - 1;
		} else {
			recent = recent - 1;
		}
		//NRF_LOG_PRINTF_DEBUG("new recent = %u, \r\n", recent);
	}
	return false;
}
