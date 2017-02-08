#include <string.h>
#include "app_error.h"

#include "ble_hci.h"
#include "nrf_log.h"
#include "ble_aas.h"
#include "ble_srv_common.h"
#include "jwb_msg_server.h"
#include "jwb_srv_common.h"
#include "messaging.h"

static void on_connect(ble_aas_t *p_aas, ble_evt_t *p_ble_evt) {
  p_aas->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
  memcpy(&p_aas->address, &p_ble_evt->evt.gap_evt.params.connected.peer_addr,
         sizeof(ble_gap_addr_t));
}

static void on_disconnect(ble_aas_t *p_aas, ble_evt_t *p_ble_evt) {
  p_aas->conn_handle = BLE_CONN_HANDLE_INVALID;
  memset(&p_aas->address, 0, sizeof(ble_gap_addr_t));
}

static void on_write(ble_aas_t *p_aas, ble_evt_t *p_ble_evt) {
  ble_gatts_evt_write_t *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
  if (p_aas->evt_handler != NULL) {
    ble_aas_evt_t evt;
    if ((p_evt_write->handle == p_aas->rx_handles.cccd_handle) &&
        (p_evt_write->len == 2)) {
      if (ble_srv_is_notification_enabled(p_evt_write->data)) {
        evt.evt_type = BLE_AAS_EVT_NOTIFICATION_ENABLED;
        p_aas->is_notification_enabled = true;
      } else {
        evt.evt_type = BLE_AAS_EVT_NOTIFICATION_ENABLED;
        p_aas->is_notification_enabled = false;
      }
    } else if (p_evt_write->handle == p_aas->tx_handles.value_handle) {
      p_aas->data_handler(p_aas, p_evt_write->data, p_evt_write->len);
    }

    p_aas->evt_handler(p_aas, &evt);
  }
}


static uint32_t rx_char_add(ble_aas_t *p_aas) {
  ble_gatts_char_md_t char_md;
  ble_gatts_attr_md_t cccd_md;
  ble_gatts_attr_t attr_char_value;
  ble_uuid_t ble_uuid;
  ble_gatts_attr_md_t attr_md;

  memset(&cccd_md, 0, sizeof(cccd_md));

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

  cccd_md.vloc = BLE_GATTS_VLOC_STACK;

  memset(&char_md, 0, sizeof(char_md));

  char_md.char_props.notify = 1;
  char_md.p_char_user_desc = NULL;
  char_md.p_char_pf = NULL;
  char_md.p_user_desc_md = NULL;
  char_md.p_cccd_md = &cccd_md;
  char_md.p_sccd_md = NULL;

  ble_uuid.type = p_aas->uuid_type;
  ble_uuid.uuid = JWB_BLE_UUID_ANNOTATED_ALERT_CHAR_RX;

  memset(&attr_md, 0, sizeof(attr_md));

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

  attr_md.vloc = BLE_GATTS_VLOC_STACK;
  attr_md.rd_auth = 0;
  attr_md.wr_auth = 0;
  attr_md.vlen = 1;

  memset(&attr_char_value, 0, sizeof(attr_char_value));

  attr_char_value.p_uuid = &ble_uuid;
  attr_char_value.p_attr_md = &attr_md;
  attr_char_value.init_len = sizeof(uint8_t);
  attr_char_value.init_offs = 0;
  attr_char_value.max_len = MAX_ALERT_SIZE;

  return sd_ble_gatts_characteristic_add(p_aas->service_handle, &char_md,
                                         &attr_char_value, &p_aas->rx_handles);
}
static uint32_t tx_char_add(ble_aas_t *p_aas) {

  // uint32_t err_code;
  ble_gatts_char_md_t char_md;
  ble_gatts_attr_md_t cccd_md;
  ble_gatts_attr_t attr_char_value;
  ble_uuid_t ble_uuid;
  ble_gatts_attr_md_t attr_md;

  memset(&cccd_md, 0, sizeof(cccd_md));

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

  cccd_md.vloc = BLE_GATTS_VLOC_STACK;

  memset(&char_md, 0, sizeof(char_md));

  char_md.char_props.read = 1;
  char_md.char_props.notify = 1;
  char_md.char_props.write = 1;
  char_md.p_char_user_desc = NULL;
  char_md.p_char_pf = NULL;
  char_md.p_user_desc_md = NULL;

  char_md.p_cccd_md = &cccd_md;

  char_md.p_sccd_md = NULL;

  ble_uuid.uuid = JWB_BLE_UUID_ANNOTATED_ALERT_CHAR_TX;
  ble_uuid.type = p_aas->uuid_type;

  memset(&attr_md, 0, sizeof(attr_md));

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

  attr_md.vloc = BLE_GATTS_VLOC_STACK;
  attr_md.rd_auth = 0;
  attr_md.wr_auth = 0;
  attr_md.vlen = 0;

  memset(&attr_char_value, 0, sizeof(attr_char_value));

  attr_char_value.p_uuid = &ble_uuid;
  attr_char_value.p_attr_md = &attr_md;
  attr_char_value.init_len = 0;
  attr_char_value.init_offs = 0;
  attr_char_value.max_len = MAX_ALERT_SIZE;

  return sd_ble_gatts_characteristic_add(p_aas->service_handle, &char_md,
                                         &attr_char_value, &p_aas->tx_handles);
}

uint32_t ble_aas_alert_set(ble_aas_t *p_aas, uint8_t *p_alert,
                           uint8_t *p_alert_length) {
  ble_gatts_value_t gatts_value;

  memset(&gatts_value, 0, sizeof(gatts_value));
  gatts_value.len = *p_alert_length;
  gatts_value.offset = 0; // todo: Should the offset really be zero? Check.
  gatts_value.p_value = p_alert;
  return sd_ble_gatts_value_set(p_aas->conn_handle,
                                p_aas->tx_handles.value_handle, &gatts_value);
}

uint32_t ble_aas_init(ble_aas_t *p_aas, const ble_aas_init_t *p_aas_init) {

  uint32_t err_code;
  ble_uuid_t ble_uuid;
  ble_uuid128_t aas_base_uuid = JWB_BLE_UUID_AAS_BASE;
  err_code = sd_ble_uuid_vs_add(&aas_base_uuid, &p_aas->uuid_type);

  memset(&p_aas->address, 0, sizeof(ble_gap_addr_t));

  p_aas->conn_handle = BLE_CONN_HANDLE_INVALID;
  p_aas->evt_handler = p_aas_init->evt_handler;
  p_aas->data_handler = p_aas_init->data_handler;
  ble_uuid.type = p_aas->uuid_type;
  ble_uuid.uuid = JWB_BLE_UUID_ANNOTATED_ALERT_SERVICE;

  err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid,
                                      &p_aas->service_handle);

  if (err_code != NRF_SUCCESS) {
    NRF_LOG_PRINTF_DEBUG("BLE_AAS_INIT ERROR ADDING GATTS SERVICE: %u\r\n", err_code);
    return err_code;
  }

  err_code = tx_char_add(p_aas);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }
  err_code = rx_char_add(p_aas);
  return err_code;
}

void ble_aas_on_ble_evt(ble_aas_t *p_aas, ble_evt_t *p_ble_evt) {
//  if (p_ble_evt->header.evt_id != BLE_GAP_EVT_ADV_REPORT) {
//    NRF_LOG_PRINTF_DEBUG("GATT SERVER, GAP PERIPHERAL EVENT: %04x\r\n",
//                   p_ble_evt->header.evt_id);
//  }
  switch (p_ble_evt->header.evt_id) {
  case BLE_GAP_EVT_CONNECTED:
    //NRF_LOG_DEBUG("GATT SERVER CONNECTED\r\n");
    on_connect(p_aas, p_ble_evt);
    break;
  case BLE_GAP_EVT_DISCONNECTED:
    //NRF_LOG_DEBUG("GATT SERVER DISCONNECTED\r\n");
    on_disconnect(p_aas, p_ble_evt);
    break;
  case BLE_GATTS_EVT_WRITE:
    NRF_LOG_DEBUG("BLE_AAS:ON WRITE!\r\n");
    on_write(p_aas, p_ble_evt);
    break;
  case BLE_GATTS_EVT_TIMEOUT:
    if (p_ble_evt->evt.gatts_evt.conn_handle != BLE_CONN_HANDLE_INVALID) {
      uint32_t err_code;
      NRF_LOG_DEBUG("BLE_GATTS_EVT_TIMEOUT\r\n");
      err_code =
          sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                BLE_HCI_LOCAL_HOST_TERMINATED_CONNECTION);
      NRF_LOG_PRINTF_DEBUG("DISCONNECT: %u\r\n", err_code);
      APP_ERROR_CHECK(err_code);
    }
    NRF_LOG_DEBUG("BLE_GATTS_EVT_TIMEOUT\r\n");
    break;
  default:
    break;
  }
}
