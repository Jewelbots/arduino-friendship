#ifndef JWB_MSG_CLIENT_H_
#define JWB_MSG_CLIENT_H_
#include <stdint.h>
void msg_client_init(void);
// uint32_t msg_client_send(uint8_t *msg, uint8_t msg_length);
uint32_t msg_client_get_msg(uint8_t *msg, uint8_t msg_length);
uint32_t send_msg_to_server(uint8_t *pending_message, uint8_t length);
#endif
