#ifndef JWB_MSG_SERVER_H_
#define JWB_MSG_SERVER_H_

#include <stdbool.h>
#include <stdint.h>


void msg_server_init(void);
ble_aas_t *get_aas(void);
void ready_msg_for_client(void);
bool notifications_enabled(void);
uint32_t send_msg_to_client(uint8_t *p_msg, uint16_t msg_length);
uint32_t msg_server_write_msg(uint8_t *msg, uint8_t msg_length);
#endif
