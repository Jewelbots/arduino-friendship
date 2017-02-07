#include <stdint.h>
#include "nrf_log.h"
#include "nrf_soc.h"

#include "power_management.h"
#include "utils.h"

static bool m_tx_power_set_pending = false;
static tx_power_management_mode_t m_current_power_mode =
    TX_POWER_MODE_BROADCAST;


static void set_broadcast_power() {
  if (flash_access_in_progress()) {
    m_tx_power_set_pending = true;
    return;
  }
  if (m_current_power_mode == TX_POWER_MODE_BROADCAST) {
		NRF_LOG_DEBUG("set power to full\r\n");
    sd_ble_gap_tx_power_set(0);
  } else if (m_current_power_mode == TX_POWER_MODE_FRIEND_FINDING) {
		NRF_LOG_DEBUG("set power to low\r\n");
    sd_ble_gap_tx_power_set(-20);
  }
}

void set_broadcast_tx_power(void) {
  m_current_power_mode = TX_POWER_MODE_BROADCAST;
	set_broadcast_power();
}

void set_friend_finding_tx_power(void) {
  m_current_power_mode = TX_POWER_MODE_FRIEND_FINDING;
	set_broadcast_power();
}

void power_management_on_sys_evt(uint32_t sys_evt) {
  switch (sys_evt) {
  case NRF_EVT_FLASH_OPERATION_SUCCESS:
  case NRF_EVT_FLASH_OPERATION_ERROR:
    if (m_tx_power_set_pending) {
      m_tx_power_set_pending = false;
      set_broadcast_power();
    }
    break;

  default:
    break;
  }
}
