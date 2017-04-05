#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "app_error.h"
#include "ble.h"
#include "ble_gatts.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "nrf_log.h"

#include "jewelbot_types.h"
#include "jwb_srv_common.h"
#include "jwb_jcs.h"
#include "utils.h"


static uint8_t m_friends_list[sizeof(friends_list_t)] = {0};

#define MAX_FRIENDS_LIST_SIZE sizeof(friends_list_t)
static void on_connect(ble_jcs_t *p_jcs, ble_evt_t *p_ble_evt) {
	p_jcs->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}
static void on_disconnect(ble_jcs_t * p_jcs, ble_evt_t * p_ble_evt) {
	p_jcs->conn_handle = BLE_CONN_HANDLE_INVALID;
}


static void on_write(ble_jcs_t * p_jcs, ble_evt_t * p_ble_evt) {

	ble_gatts_evt_write_t *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
	ble_gatts_evt_write_t *p_evt_write_auth = &p_ble_evt->evt.gatts_evt.params.authorize_request.request.write;

	uint8_t  friends_list_val_buf[MAX_FRIENDS_LIST_SIZE];
	ble_gatts_value_t gatts_value;

	memset(&gatts_value, 0, sizeof(gatts_value));

	gatts_value.len     = MAX_FRIENDS_LIST_SIZE;
	gatts_value.offset  = 0;
	gatts_value.p_value = friends_list_val_buf;
	
	if (p_jcs->evt_handler != NULL) {
		ble_jcs_evt_t evt;
		if (p_evt_write->op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) {
			NRF_LOG_DEBUG("LONG ON WRITE TO TX_HANDLE\r\n");
			sd_ble_gatts_value_get(p_jcs->conn_handle, p_jcs->tx_handles.value_handle, &gatts_value);
			p_jcs->data_handler(p_jcs, gatts_value.p_value, gatts_value.len);
			evt.evt_type = BLE_JCS_EVT_FRIEND_LIST_UPDATED;
		}
		else if (p_evt_write->handle == p_jcs->tx_handles.value_handle) {
			NRF_LOG_DEBUG("SHORT ON WRITE TO TX_HANDLE\r\n");
			p_jcs->data_handler(p_jcs, p_evt_write->data, p_evt_write->len);
			evt.evt_type = BLE_JCS_EVT_FRIEND_LIST_UPDATED;
		}
    else if (p_evt_write_auth->handle == p_jcs->dfu_handles.value_handle) {
      if(*(p_evt_write_auth->data) == 0xAA) {
        evt.evt_type = BLE_JCS_EVT_FIRMWARE_UPDATE_REQUESTED;
      }
    }
		p_jcs->evt_handler(p_jcs, &evt);
	}
}

static void on_rw_authorize_req(ble_jcs_t * p_jcs, ble_evt_t * p_ble_evt)
{
    ble_gatts_evt_rw_authorize_request_t * p_authorize_request;

    p_authorize_request = &(p_ble_evt->evt.gatts_evt.params.authorize_request);

    if (
        (p_authorize_request->type == BLE_GATTS_AUTHORIZE_TYPE_WRITE)
        &&
        (p_authorize_request->request.write.handle == p_jcs->dfu_handles.value_handle)
        && 
        (p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.op != BLE_GATTS_OP_PREP_WRITE_REQ)
        &&
        (p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.op != BLE_GATTS_OP_EXEC_WRITE_REQ_NOW)
        &&
        (p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.op != BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL)
       )
    {
        NRF_LOG_DEBUG("ON_RW_AUTHORIZE_REQ\r\n");
        on_write(p_jcs, p_ble_evt);
    }
}



//static uint32_t rx_char_add(ble_jcs_t *p_jcs) {
//  ble_gatts_char_md_t char_md;
//  ble_gatts_attr_md_t cccd_md;
//  ble_gatts_attr_t attr_char_value;
//  ble_uuid_t ble_uuid;
//  ble_gatts_attr_md_t attr_md;

//  memset(&cccd_md, 0, sizeof(cccd_md));

//  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
//  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

//  cccd_md.vloc = BLE_GATTS_VLOC_STACK;

//  memset(&char_md, 0, sizeof(char_md));

//  char_md.char_props.notify = 1;
//  char_md.p_char_user_desc = NULL;
//  char_md.p_char_pf = NULL;
//  char_md.p_user_desc_md = NULL;
//  char_md.p_cccd_md = &cccd_md;
//  char_md.p_sccd_md = NULL;

//  ble_uuid.type = p_jcs->uuid_type;
//  ble_uuid.uuid = JWB_BLE_UUID_FRIENDS_LIST_CHAR_RX;

//  memset(&attr_md, 0, sizeof(attr_md));

//  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
//  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

//  attr_md.vloc = BLE_GATTS_VLOC_STACK;
//  attr_md.rd_auth = 0;
//  attr_md.wr_auth = 0;
//  attr_md.vlen = 1;

//  memset(&attr_char_value, 0, sizeof(attr_char_value));

//  attr_char_value.p_uuid = &ble_uuid;
//  attr_char_value.p_attr_md = &attr_md;
//  attr_char_value.init_len = sizeof(uint8_t);
//  attr_char_value.init_offs = 0;
//  attr_char_value.max_len = MAX_FRIENDS_LIST_SIZE;

//  return sd_ble_gatts_characteristic_add(p_jcs->service_handle, &char_md,
//                                         &attr_char_value, &p_jcs->rx_handles);
//}

static uint32_t dfu_char_add(ble_jcs_t * p_jcs) {
  
  
  ble_uuid_t ble_uuid;
	ble_uuid128_t jcs_base_uuid = JWB_BLE_UUID_JCS_BASE;
	ble_uuid.uuid = JWB_BLE_UUID_DFU_UPDATE_CHAR_RX;
  ble_uuid.type = p_jcs->uuid_type;
	sd_ble_uuid_vs_add(&jcs_base_uuid, &p_jcs->uuid_type);

	ble_gatts_char_md_t char_md;
	memset(&char_md, 0, sizeof(char_md));

  char_md.char_props.write = 1;
  char_md.p_char_user_desc = NULL;
  char_md.p_char_pf = NULL;
  char_md.p_user_desc_md = NULL;
	
//  ble_gatts_attr_md_t cccd_md;
//	memset(&cccd_md, 0, sizeof(cccd_md));
//	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
//  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&cccd_md.write_perm);
//	
//  cccd_md.vloc = BLE_GATTS_VLOC_STACK;
//  char_md.p_cccd_md = &cccd_md;
//  char_md.p_sccd_md = NULL;
//	char_md.char_props.notify   = 1;
	
	ble_gatts_attr_md_t attr_md;
	memset(&attr_md, 0, sizeof(attr_md));

  BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&attr_md.write_perm);

  attr_md.vloc = BLE_GATTS_VLOC_STACK;
  attr_md.rd_auth = 0;
  attr_md.wr_auth = 1;
  attr_md.vlen = 1;
	
  ble_gatts_attr_t attr_char_value;
  memset(&attr_char_value, 0, sizeof(attr_char_value));

  attr_char_value.p_uuid = &ble_uuid;
  attr_char_value.p_attr_md = &attr_md;
  attr_char_value.init_len = sizeof(uint8_t);
  attr_char_value.init_offs = 0;
  attr_char_value.max_len = sizeof(uint8_t); 
  attr_char_value.p_value = 0;

  return sd_ble_gatts_characteristic_add(p_jcs->service_handle, &char_md,
                                         &attr_char_value, &p_jcs->dfu_handles);
}
static uint32_t tx_char_add(ble_jcs_t *p_jcs) {

	ble_uuid_t ble_uuid;
	ble_uuid128_t jcs_base_uuid = JWB_BLE_UUID_JCS_BASE;
	ble_uuid.uuid = JWB_BLE_UUID_FRIENDS_LIST_CHAR_TX;
  ble_uuid.type = p_jcs->uuid_type;
	sd_ble_uuid_vs_add(&jcs_base_uuid, &p_jcs->uuid_type);
	
  ble_gatts_char_md_t char_md;
	memset(&char_md, 0, sizeof(char_md));

  char_md.char_props.read = 1;
  char_md.char_props.write = 1;
  char_md.p_char_user_desc = NULL;
  char_md.p_char_pf = NULL;
  char_md.p_user_desc_md = NULL;
	
//	ble_gatts_attr_md_t cccd_md;
//	memset(&cccd_md, 0, sizeof(cccd_md));

//  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
//  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

//  cccd_md.vloc = BLE_GATTS_VLOC_STACK;
//	char_md.p_cccd_md = &cccd_md;
//  char_md.p_sccd_md = NULL;
//	char_md.char_props.notify   = 1;
	
	ble_gatts_attr_md_t attr_md;
  memset(&attr_md, 0, sizeof(attr_md));

  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&attr_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&attr_md.write_perm);
	
	attr_md.vloc = BLE_GATTS_VLOC_STACK;
  attr_md.rd_auth = 0;
  attr_md.wr_auth = 0;
  attr_md.vlen = 0;
	
  ble_gatts_attr_t attr_char_value;

  memset(&attr_char_value, 0, sizeof(attr_char_value));

  attr_char_value.p_uuid = &ble_uuid;
  attr_char_value.p_attr_md = &attr_md;
  attr_char_value.init_len = MAX_FRIENDS_LIST_SIZE;
  attr_char_value.init_offs = 0;
  attr_char_value.max_len = MAX_FRIENDS_LIST_SIZE;
	attr_char_value.p_value = &m_friends_list[0];
	
  return sd_ble_gatts_characteristic_add(p_jcs->service_handle, &char_md,
                                         &attr_char_value, &p_jcs->tx_handles);
}

uint32_t ble_jcs_friends_list_set(ble_jcs_t *p_jcs, friends_list_t * p_list,
                           uint8_t friends_list_length) {
  memcpy(m_friends_list, p_list, friends_list_length);
  ble_gatts_value_t gatts_value;

  memset(&gatts_value, 0, sizeof(gatts_value));
  gatts_value.len = friends_list_length;
  gatts_value.offset = 0; // todo: Should the offset really be zero? Check.
  gatts_value.p_value = &m_friends_list[0];
  return sd_ble_gatts_value_set(p_jcs->conn_handle,
                                p_jcs->tx_handles.value_handle, &gatts_value);
}

uint32_t ble_jcs_init(ble_jcs_t *p_jcs, const ble_jcs_init_t *p_jcs_init) {

  uint32_t err_code;
  ble_uuid_t ble_uuid;
  ble_uuid128_t jcs_base_uuid = JWB_BLE_UUID_JCS_BASE;
  err_code = sd_ble_uuid_vs_add(&jcs_base_uuid, &p_jcs->uuid_type);

  p_jcs->conn_handle = BLE_CONN_HANDLE_INVALID;
  p_jcs->evt_handler = p_jcs_init->evt_handler;
  p_jcs->data_handler = p_jcs_init->data_handler;
  ble_uuid.type = p_jcs->uuid_type;
  ble_uuid.uuid = JWB_BLE_UUID_FRIENDS_LIST_SERVICE;

  err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid,
                                      &p_jcs->service_handle);

  if (err_code != NRF_SUCCESS) {
    NRF_LOG_PRINTF_DEBUG("BLE_JCS_INIT ERROR ADDING GATTS SERVICE: %u\r\n", err_code);
    return err_code;
  }

  err_code = tx_char_add(p_jcs);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }
  
//  err_code = rx_char_add(p_jcs);
//  if (err_code != NRF_SUCCESS) {
//    return err_code;
//  }
  
  err_code = dfu_char_add(p_jcs);
  return err_code;
}

void ble_jcs_on_ble_evt(ble_jcs_t *p_jcs, ble_evt_t *p_ble_evt) {
//  if (p_ble_evt->header.evt_id != BLE_GAP_EVT_ADV_REPORT) {
//    NRF_LOG_PRINTF_DEBUG("GATT SERVER, GAP PERIPHERAL EVENT: %04x\r\n",
//                   p_ble_evt->header.evt_id);
//  }
  switch (p_ble_evt->header.evt_id) {
  case BLE_GAP_EVT_CONNECTED:
    //NRF_LOG_DEBUG("GATT SERVER CONNECTED\r\n");
    on_connect(p_jcs, p_ble_evt);
    break;
  case BLE_GAP_EVT_DISCONNECTED:
    //NRF_LOG_DEBUG("GATT SERVER DISCONNECTED\r\n");
    on_disconnect(p_jcs, p_ble_evt);
    break;
  case BLE_GATTS_EVT_WRITE:
    NRF_LOG_DEBUG("BLE_JCS:ON WRITE!\r\n");
    on_write(p_jcs, p_ble_evt);
    break;
  case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
    on_rw_authorize_req(p_jcs, p_ble_evt);
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
