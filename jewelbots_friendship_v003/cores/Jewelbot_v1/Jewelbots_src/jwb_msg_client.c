#include <string.h>
#include "app_error.h"
#include "ble_aas_c.h"
#include "ble_central_event_handler.h"
#include "fsm.h"
#include "jwb_msg_client.h"
#include "led_sequence.h"
#include "messaging.h"
#include "utils.h"

static ble_aas_c_t m_aas_c;
static volatile bool m_is_aas_present = false;

static void check_for_messages_to_send() {
  if (has_message_to_send()) {
    NRF_LOG_PRINTF_DEBUG("current msg color cycle is: %u\r\n",
                   get_current_msg_color_cycle());
    send_message(get_current_msg_color_cycle(), true);
    reset_current_msg_color_cycle(); // reset cycle back to zero
  }
}

static void on_aas_c_evt(ble_aas_c_t *p_aas_c, ble_aas_c_evt_t *p_evt) {
  uint32_t err_code = NRF_SUCCESS;

  switch (p_evt->evt_type) {
  case BLE_AAS_C_EVT_DISCOVERY_COMPLETE:
    m_is_aas_present = true;
    err_code =
        ble_aas_c_handles_assign(p_aas_c, p_evt->conn_handle, &p_evt->handles);
    err_code = ble_aas_c_notif_source_notif_enable(p_aas_c);
    NRF_LOG_PRINTF_DEBUG("DISCOVERY COMPLETE. ENABLED NOTIFICATIONS: %x\r\n",
                   err_code);

    check_for_messages_to_send();
    break; // BLE_AAS_C_EVT_DISCOVERY_COMPLETE
  case BLE_AAS_C_EVT_ALERT_READ_RESP:
    NRF_LOG_DEBUG("MSG_RECEIVED?: ");
    for (uint8_t i = 0; i < p_evt->alert_length; i++) {
      NRF_LOG_PRINTF_DEBUG(" %x", p_evt->p_annotated_alert[i]);
    }
    NRF_LOG_DEBUG("\r\n");
    NRF_LOG_PRINTF_DEBUG("PRINT ALERT LENGTH: %u\r\n", p_evt->alert_length);
    break;
  case BLE_AAS_C_EVT_DISCOVERY_FAILED:
    NRF_LOG_PRINTF_DEBUG("Discovery FAILED! \r\n");
    disconnect();
    break; // BLE_AAS_C_EVT_DISCOVERY_FAILED
  case BLE_AAS_C_EVT_AAS_RX_EVT:
    queue_message_for_play((uint8_t *)p_evt->p_annotated_alert,
                           p_evt->alert_length);
    NRF_LOG_DEBUG("RECEIVED STUFF\r\n");
    set_jwb_event_signal(MSG_RECEIVED_SIG);
    break;
  case BLE_AAS_C_EVT_DISCONN_COMPLETE:
    // Disable alert buttons
    m_is_aas_present = false;
    memset(&m_aas_c.address, 0, sizeof(m_aas_c.address));
    break; // BLE_AAS_C_EVT_DISCONN_COMPLETE
  default:

    break;
  }
  UNUSED_PARAMETER(err_code);
}

static void service_error_handler(uint32_t nrf_error) {
  NRF_LOG_PRINTF_DEBUG("MSG CLIENT ERROR: 0x%04x\r\n", nrf_error);
  APP_ERROR_HANDLER(nrf_error);
}
static void aas_client_init(void) {
  uint32_t err_code;
  ble_aas_c_init_t aas_c_init_obj;

  memset(&aas_c_init_obj, 0, sizeof(aas_c_init_obj));

  aas_c_init_obj.evt_handler = on_aas_c_evt;
  aas_c_init_obj.error_handler = service_error_handler;

  err_code = ble_aas_c_init(&m_aas_c, &aas_c_init_obj);
  APP_ERROR_CHECK(err_code);
}
uint32_t send_msg_to_server(uint8_t *pending_message, uint8_t length) {
  return ble_aas_c_alert_send(&m_aas_c, pending_message, length);
}

void msg_client_init() { aas_client_init(); }
ble_aas_c_t *get_aas_c(void) { return &m_aas_c; }
