#include <string.h>
#include <stdint.h>
#include "ble.h"
#include "ble_db_discovery.h"
#include "ble_gattc.h"
#include "ble_srv_common.h"
#include "sdk_common.h"
#include "messaging.h"

#include "ble_aas_c.h"
#include "jwb_srv_common.h"


#define TX_BUFFER_MASK 0x07
#define BLE_CCCD_NOTIFY_BIT_MASK 0x0001


static tx_message_t m_tx_buffer[MAX_MSG_LENGTH];
static uint32_t m_tx_index = 0;
static uint32_t m_tx_insert_index = 0;
static void tx_buffer_process(void);

static uint32_t cccd_configure(const uint16_t conn_handle,
                               const uint16_t cccd_handle, bool enable) {
  tx_message_t *p_msg;
  uint16_t cccd_val = enable ? BLE_CCCD_NOTIFY_BIT_MASK : 0;

  p_msg = &m_tx_buffer[m_tx_insert_index++];
  m_tx_insert_index &= TX_BUFFER_MASK;

  p_msg->req.write_req.gattc_params.handle = cccd_handle;
  p_msg->req.write_req.gattc_params.len = 2;
  p_msg->req.write_req.gattc_params.p_value = p_msg->req.write_req.gattc_value;
  p_msg->req.write_req.gattc_params.offset = 0;
  p_msg->req.write_req.gattc_params.write_op = BLE_GATT_OP_WRITE_REQ;
  p_msg->req.write_req.gattc_params.flags =
      BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE;
  p_msg->req.write_req.gattc_value[0] = LSB_16(cccd_val);
  p_msg->req.write_req.gattc_value[1] = MSB_16(cccd_val);
  p_msg->conn_handle = conn_handle;
  p_msg->type = WRITE_REQ;

  tx_buffer_process();
  return NRF_SUCCESS;
}


static void on_connect(ble_aas_c_t *p_aas_c, ble_evt_t const *p_ble_evt) {
  memcpy(&p_aas_c->address, &p_ble_evt->evt.gap_evt.params.connected.peer_addr,
         sizeof(ble_gap_addr_t));
}

static void on_disconnect(ble_aas_c_t *p_aas_c, ble_evt_t const *p_ble_evt) {
  p_aas_c->conn_handle = BLE_CONN_HANDLE_INVALID;
  memset(&p_aas_c->address, 0, sizeof(ble_gap_addr_t));
  if (p_ble_evt->evt.gap_evt.conn_handle == p_aas_c->conn_handle) {
    ble_aas_c_evt_t evt;

    evt.evt_type = BLE_AAS_C_EVT_DISCONN_COMPLETE;

    p_aas_c->evt_handler(p_aas_c, &evt);
    p_aas_c->handles.aas_rx_handle = BLE_GATT_HANDLE_INVALID;
    p_aas_c->handles.aas_tx_handle = BLE_GATT_HANDLE_INVALID;
  }
  //NRF_LOG_DEBUG("GATT CLIENT: DISCONNECT\r\n");
}

static void on_hvx(ble_aas_c_t *p_aas_c, const ble_evt_t *p_ble_evt) {
  if ((p_aas_c->handles.aas_rx_handle != BLE_GATT_HANDLE_INVALID) &&
      (p_ble_evt->evt.gattc_evt.params.hvx.handle ==
       p_aas_c->handles.aas_rx_handle) &&
      (p_aas_c->evt_handler != NULL)) {
    ble_aas_c_evt_t ble_aas_c_evt;
    ble_aas_c_evt.evt_type = BLE_AAS_C_EVT_AAS_RX_EVT;
    ble_aas_c_evt.p_annotated_alert =
        (uint16_t *)p_ble_evt->evt.gattc_evt.params.hvx.data;
    ble_aas_c_evt.alert_length =
        (uint16_t)p_ble_evt->evt.gattc_evt.params.hvx.len;

    p_aas_c->evt_handler(p_aas_c, &ble_aas_c_evt);
  }
}

static void on_read_resp(ble_aas_c_t *p_aas_c, const ble_evt_t *p_ble_evt) {
  const ble_gattc_evt_read_rsp_t *p_response;

  if (p_aas_c->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle) {
    return;
  }

  p_response = &p_ble_evt->evt.gattc_evt.params.read_rsp;
  NRF_LOG_DEBUG("IN READ RESPONSE\r\n");
  if (p_response->handle == p_aas_c->handles.aas_tx_handle) {
    ble_aas_c_evt_t evt;

    evt.conn_handle = p_ble_evt->evt.gattc_evt.conn_handle;
    evt.evt_type = BLE_AAS_C_EVT_ALERT_READ_RESP;

    evt.alert_length = p_response->len;
    NRF_LOG_PRINTF_DEBUG("p_response->len %u\r\n", p_response->len);
    for (uint8_t i = 0; i < p_response->len; i++) {
      NRF_LOG_PRINTF_DEBUG("%x ", p_response->data[i]);
    }
    NRF_LOG_PRINTF_DEBUG("offset: %u\r\n", p_response->offset);

    memcpy(evt.p_annotated_alert, p_response->data, p_response->len);
    p_aas_c->evt_handler(p_aas_c, &evt);
  }
  tx_buffer_process();
}

static void on_write_rsp(ble_aas_c_t *p_aas_c, const ble_evt_t *p_ble_evt) {
  if (p_aas_c->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle) {
    NRF_LOG_DEBUG("NOT THE ONE WE CARE ABOUT\r\n");
    return;
  }
  NRF_LOG_DEBUG("Write Response Received.\r\n");
  tx_buffer_process();
}

static void tx_buffer_process(void) {
  if (m_tx_index != m_tx_insert_index) {
    uint32_t err_code;

    if (m_tx_buffer[m_tx_index].type == READ_REQ) {
      err_code = sd_ble_gattc_read(m_tx_buffer[m_tx_index].conn_handle,
                                   m_tx_buffer[m_tx_index].req.read_handle, 0);
      NRF_LOG_PRINTF_DEBUG("READ_REQ: %u\r\n", err_code);
    } else {
      err_code = sd_ble_gattc_write(
          m_tx_buffer[m_tx_index].conn_handle,
          &m_tx_buffer[m_tx_index].req.write_req.gattc_params);
      NRF_LOG_PRINTF_DEBUG("WRITE_REQ: %u\r\n", err_code);
    }
    if (err_code == NRF_SUCCESS) {
      ++m_tx_index;
      m_tx_index &= TX_BUFFER_MASK;
    }
  }
}

void ble_aas_c_on_db_disc_evt(ble_aas_c_t *p_aas_c,
                              const ble_db_discovery_evt_t *p_evt) {
  ble_aas_c_evt_t evt;
  memset(&evt, 0, sizeof(ble_aas_c_evt_t));
  evt.evt_type = BLE_AAS_C_EVT_DISCOVERY_FAILED;
  evt.conn_handle = p_evt->conn_handle;

  NRF_LOG_PRINTF_DEBUG("DISCOVERY STATUS: %u\r\n", p_evt->evt_type);
  if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE &&
      p_evt->params.discovered_db.srv_uuid.uuid ==
          JWB_BLE_UUID_ANNOTATED_ALERT_SERVICE &&
      p_evt->params.discovered_db.srv_uuid.type == p_aas_c->service_uuid.type) {

    uint32_t i;
    const ble_gatt_db_char_t *p_chars =
        p_evt->params.discovered_db.charateristics;
    for (i = 0; i < p_evt->params.discovered_db.char_count; i++) {
      switch (p_chars[i].characteristic.uuid.uuid) {
      case JWB_BLE_UUID_ANNOTATED_ALERT_CHAR_TX:
        evt.handles.aas_tx_handle = p_chars[i].characteristic.handle_value;
        NRF_LOG_PRINTF_DEBUG("FOUND JWB_BLE_UUID_ANNOTATED_ALERT_CHAR_TX, %u\r\n",
                       p_chars[i].characteristic.handle_value);
        break;
      case JWB_BLE_UUID_ANNOTATED_ALERT_CHAR_RX:
        evt.handles.aas_rx_handle = p_chars[i].characteristic.handle_value;
        evt.handles.aas_rx_cccd_handle = p_chars[i].cccd_handle;
        NRF_LOG_PRINTF_DEBUG("FOUND JWB_BLE_UUID_ANNOTATED_ALERT_CHAR_RX, %u, %u\r\n",
                       p_chars[i].characteristic.handle_value,
                       p_chars[i].cccd_handle);
        break;
      default:
        break;
      }
    }
    if (p_aas_c->evt_handler != NULL) {
      evt.conn_handle = p_evt->conn_handle;
      evt.evt_type = BLE_AAS_C_EVT_DISCOVERY_COMPLETE;
    }
  }
  p_aas_c->evt_handler(p_aas_c, &evt);
}


uint32_t ble_aas_c_handles_assign(ble_aas_c_t *p_aas_c,
                                  const uint16_t conn_handle,
                                  const ble_aas_c_handles_t *p_peer_handles) {
  p_aas_c->conn_handle = conn_handle;
  p_aas_c->handles.aas_rx_handle = p_peer_handles->aas_rx_handle;
  p_aas_c->handles.aas_rx_cccd_handle = p_peer_handles->aas_rx_cccd_handle;
  p_aas_c->handles.aas_tx_handle = p_peer_handles->aas_tx_handle;
  return NRF_SUCCESS;
}

uint32_t ble_aas_c_init(ble_aas_c_t *p_aas_c,
                        ble_aas_c_init_t const *p_aas_c_init) {

  memset(m_tx_buffer, 0, MAX_MSG_LENGTH);
  memset(&p_aas_c->address, 0, sizeof(ble_gap_addr_t));

  ble_uuid128_t aas_base_uuid = JWB_BLE_UUID_AAS_BASE;
  uint32_t err_code =
      sd_ble_uuid_vs_add(&aas_base_uuid, &p_aas_c->service_uuid.type);
  APP_ERROR_CHECK(err_code);

  VERIFY_PARAM_NOT_NULL(p_aas_c);
  VERIFY_PARAM_NOT_NULL(p_aas_c_init->evt_handler);
  VERIFY_PARAM_NOT_NULL(p_aas_c_init);

  p_aas_c->service_uuid.uuid = JWB_BLE_UUID_ANNOTATED_ALERT_SERVICE;

  p_aas_c->evt_handler = p_aas_c_init->evt_handler;
  p_aas_c->error_handler = p_aas_c_init->error_handler;
  p_aas_c->conn_handle = BLE_CONN_HANDLE_INVALID;
  p_aas_c->handles.aas_rx_handle = BLE_GATT_HANDLE_INVALID;
  p_aas_c->handles.aas_tx_handle = BLE_GATT_HANDLE_INVALID;

  return ble_db_discovery_evt_register(&p_aas_c->service_uuid);
}

uint32_t ble_aas_c_alert_send(ble_aas_c_t *p_aas_c, uint8_t *p_alert,
                              uint16_t length) {
  VERIFY_PARAM_NOT_NULL(p_aas_c);
  if (length > MAX_MSG_LENGTH) {
    return NRF_ERROR_INVALID_PARAM;
  }
  if (p_aas_c->conn_handle == BLE_CONN_HANDLE_INVALID) {
    return NRF_ERROR_INVALID_STATE;
  }

  tx_message_t *p_msg;
  p_msg = &m_tx_buffer[m_tx_insert_index++];
  m_tx_insert_index &= TX_BUFFER_MASK;

  p_msg->req.write_req.gattc_params.handle = p_aas_c->handles.aas_tx_handle;
  p_msg->req.write_req.gattc_params.len = length;
  p_msg->req.write_req.gattc_params.p_value = p_alert;
  p_msg->req.write_req.gattc_params.offset = 0;
  p_msg->req.write_req.gattc_params.write_op = BLE_GATT_OP_WRITE_CMD;
  p_msg->req.write_req.gattc_params.flags =
      BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE;

  p_msg->conn_handle = p_aas_c->conn_handle;
  p_msg->type = WRITE_REQ;

  tx_buffer_process();
  return NRF_SUCCESS;
}

uint32_t ble_aas_c_notif_source_notif_enable(const ble_aas_c_t *p_aas_c) {

  return cccd_configure(p_aas_c->conn_handle,
                        p_aas_c->handles.aas_rx_cccd_handle, true);
}

void ble_aas_c_on_ble_evt(ble_aas_c_t *p_aas_c, ble_evt_t const *p_ble_evt) {
  uint32_t err_code = NRF_SUCCESS;

  switch (p_ble_evt->header.evt_id) {
  case BLE_GAP_EVT_DISCONNECTED:
    on_disconnect(p_aas_c, p_ble_evt);
    break;
  case BLE_GAP_EVT_CONNECTED:
    on_connect(p_aas_c, p_ble_evt);
    break;
  case BLE_GATTC_EVT_READ_RSP:
    on_read_resp(p_aas_c, p_ble_evt);
    break;
  case BLE_GATTC_EVT_WRITE_RSP:
    on_write_rsp(p_aas_c, p_ble_evt);
    break;
  case BLE_GATTC_EVT_HVX:
    on_hvx(p_aas_c, p_ble_evt);
    break;
  default:
    break;
  }

  if (err_code != NRF_SUCCESS && p_aas_c->error_handler != 0) {
    p_aas_c->error_handler(err_code);
  }
}
