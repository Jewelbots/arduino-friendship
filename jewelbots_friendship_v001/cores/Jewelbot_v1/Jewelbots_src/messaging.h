#ifndef MESSAGING_H_
#define MESSAGING_H_
#include <stdint.h>
#include <stdbool.h>
#include "ble_gap.h"

#define MSG_LONG 0x02
#define MSG_SHORT 0x01

#define MSG_BUFFER_SIZE 18
#define MAX_MSG_SIZE MSG_BUFFER_SIZE

#define MAX_MSG_LENGTH MSG_COLOR_SIZE + MSG_LENGTH_SIZE + MSG_BUFFER_SIZE
#define MSG_COLOR_SIZE 1
#define MSG_LENGTH_SIZE 1
#define MAX_ALERT_SIZE MAX_MSG_LENGTH
#define MSG_START_INDEX 1
#define COLOR_INDEX 0
#define MSG_START_INDEX_2 2
#define COLOR_INDEX_2 1
#define SIZE_INDEX 19
#define MSG_PACKET_SIZE 20
#define MSG_TAG_LENGTH 2
#define MSG_TAG_QUEUE_LENGTH 4
#define MSG_PADDING 2
#define MSG_REC_FLAG_INDEX 2
#define MSG_REC_START_INDEX 3


typedef struct {
  uint8_t color_index;
  uint8_t msg[MAX_MSG_SIZE];
  uint8_t msg_length;
} jwb_message_t;

bool has_message_to_play(void);
bool has_message_to_send(void);
void messaging_init(void);
void msg_queue_pop(uint8_t color_index, uint8_t msg[], uint8_t *size);
void msg_queue_push(uint8_t color_index, uint8_t msg_bit);
bool pending_message_for_friend(ble_gap_addr_t *address);
bool pending_message_for_friend_index(uint8_t friend_index);
void play_message(void);
void queue_message_for_play(uint8_t *msg, uint8_t msg_length);
void record_msg_tag(uint8_t msg_tag[MSG_TAG_LENGTH]);
bool msg_tag_in_queue(uint8_t msg_tag[MSG_TAG_LENGTH]);

#endif
