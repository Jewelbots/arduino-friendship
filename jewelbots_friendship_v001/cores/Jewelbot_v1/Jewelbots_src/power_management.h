#ifndef POWER_MANAGEMENT_H
#define POWER_MANAGEMENT_H
#include <stdint.h>

void set_broadcast_tx_power(void);
void set_friend_finding_tx_power(void);
void power_management_on_sys_evt(uint32_t sys_evt);
typedef enum {
  TX_POWER_MODE_BROADCAST,
  TX_POWER_MODE_FRIEND_FINDING
} tx_power_management_mode_t;
#endif
