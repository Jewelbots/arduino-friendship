#include <stdint.h>
#include <string.h>
#include "app_error.h"
#include "ble.h"
#include "ble_srv_common.h"

#include "ble_aas.h"
#include "fsm.h"
#include "jwb_msg_server.h"
#include "jwb_srv_common.h"
#include "led_sequence.h"
#include "messaging.h"

static ble_aas_t m_aas;

ble_aas_t *get_aas(void) { return &m_aas; }

static void aas_data_handler(ble_aas_t *p_aas, uint8_t *data, uint8_t length) {
  queue_message_for_play(data, length);
  NRF_LOG_DEBUG("RECEIVED STUFF\r\n");
  set_jwb_event_signal(MSG_RECEIVED_SIG);
}
static void on_aas_evt(ble_aas_t *p_aas, ble_aas_evt_t *p_evt) {
  switch (p_evt->evt_type) {
  case BLE_AAS_EVT_NOTIFICATION_ENABLED:
    NRF_LOG_DEBUG("NOTIFICATIONS ENABLED!!\r\n");
    ready_msg_for_client();
    break;
  }
}

static void aas_init(void) {
  uint32_t err_code;
  ble_aas_init_t aas_init_obj;

  memset(&aas_init_obj, 0, sizeof(ble_aas_init_t));
  aas_init_obj.evt_handler = on_aas_evt;
  aas_init_obj.data_handler = aas_data_handler;
  err_code = ble_aas_init(&m_aas, &aas_init_obj);
  APP_ERROR_CHECK(err_code);
}
void msg_server_init(void) { 
  aas_init(); 
}

uint32_t msg_server_write_msg(uint8_t *msg, uint8_t msg_length) {
  if (pending_message_for_friend(&m_aas.address)) {
    uint32_t err_code = ble_aas_alert_set(&m_aas, msg, &msg_length);
    APP_ERROR_CHECK(err_code);
    if (err_code == NRF_SUCCESS) {
      NRF_LOG_PRINTF_DEBUG("trying to send alert %u\r\n", err_code);
      if (err_code == NRF_SUCCESS) {
        return NRF_SUCCESS;
      }
    } else {
      NRF_LOG_PRINTF_DEBUG("failed to send alert: %u\r\n", err_code);
    }
    APP_ERROR_CHECK(err_code);
    return err_code;
  }
  NRF_LOG_PRINTF_DEBUG("No friend to message.\r\n");
  return NRF_ERROR_FORBIDDEN; // not allowed to see this message
}

bool notifications_enabled() { 
  return m_aas.is_notification_enabled; 
}


void ready_msg_for_client(void) {
  if (has_message_to_send()) {
    NRF_LOG_PRINTF_DEBUG("current msg color cycle is: %u\r\n",
                   get_current_msg_color_cycle());
    send_message(get_current_msg_color_cycle(), false);
    reset_current_msg_color_cycle(); // reset cycle back to zero
  } else {
		NRF_LOG_DEBUG("pending message not for connected friend\r\n");
	}
  set_jwb_event_signal(JEWELBOT_ON_SIG);
}

uint32_t send_msg_to_client(uint8_t *p_msg, uint16_t msg_length) {
  ble_gatts_hvx_params_t hvx_params;
  if ((m_aas.conn_handle == BLE_CONN_HANDLE_INVALID) ||
      (!m_aas.is_notification_enabled)) {
    return NRF_ERROR_INVALID_STATE;
  }

  if (msg_length > MAX_MSG_LENGTH) {
    return NRF_ERROR_INVALID_PARAM;
  }
  ble_gatts_value_t gatts_value;
  memset(&gatts_value, 0, sizeof(gatts_value));
  gatts_value.len = msg_length;
  gatts_value.offset = 0;
  gatts_value.p_value = p_msg;

  uint32_t err_code = sd_ble_gatts_value_set(
      m_aas.conn_handle, m_aas.rx_handles.value_handle, &gatts_value);

  memset(&hvx_params, 0, sizeof(hvx_params));

  hvx_params.handle = m_aas.rx_handles.value_handle;
  hvx_params.p_data = gatts_value.p_value;
  hvx_params.p_len = &gatts_value.len;
  hvx_params.offset = gatts_value.offset;
  hvx_params.type = BLE_GATT_HVX_NOTIFICATION;

  return sd_ble_gatts_hvx(m_aas.conn_handle, &hvx_params);
}


