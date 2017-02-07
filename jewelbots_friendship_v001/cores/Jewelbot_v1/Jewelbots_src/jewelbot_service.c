#include <string.h>
#include <stdint.h>
#include "app_error.h"
#include "dfu_handler.h"
#include "friend_data_storage.h"
#include "fsm.h"
#include "nordic_common.h"
#include "jewelbot_service.h"
#include "jewelbot_types.h"
#include "jwb_jcs.h"

static ble_jcs_t m_jcs;
ble_jcs_t * get_jcs() {
	return &m_jcs;
}
static void update_friends_handler(ble_jcs_t * p_jcs, uint8_t * data, uint16_t length) {
  //UNUSED_PARAMETER(p_jcs);
  friends_list_t *m_friends_list;
	m_friends_list = get_current_friends_list();
	NRF_LOG_PRINTF_DEBUG("length = %u\r\n", length);
//	for (uint8_t i=0; i<length; i++) {
//		NRF_LOG_PRINTF_DEBUG("%u,  %u\r\n", i, data[i]);
//	}
	memcpy(m_friends_list, data, length);
	//update_friends_list(m_friends_list);
  save_friends(m_friends_list);
	set_current_friends_list(m_friends_list);
}

static void jewelbot_service_evt_handler(ble_jcs_t * p_jcs, ble_jcs_evt_t * p_evt) {
	UNUSED_PARAMETER(p_jcs);
  switch(p_evt->evt_type) {
    case BLE_JCS_EVT_NOTIFICATION_ENABLED:
      break;
    case BLE_JCS_EVT_NOTIFICATION_DISABLED:
      break;
    case BLE_JCS_EVT_FRIEND_LIST_UPDATED:
			NRF_LOG_DEBUG("WE WERE REQUESTED TO UPDATE FRIENDS LIST.\r\n");
      break;
	case BLE_JCS_EVT_FIRMWARE_UPDATE_REQUESTED:
      NRF_LOG_DEBUG("WE WERE REQUESTED TO UPDATE FIRMWARE.\r\n");
			bootloader_start();
      break;
  }
}

//ble_jcs_friends_list_set
void update_friends_list(friends_list_t * friends_list) {
  NRF_LOG_DEBUG("UPDATE_FRIENDS_LIST_SERVICE\r\n");
  ble_jcs_friends_list_set(&m_jcs, friends_list, sizeof(friends_list_t));
}
void jewelbot_service_init() {
	memset(&m_jcs, 0, sizeof(ble_jcs_t));
	ble_jcs_init_t jcs_init_obj;
	jcs_init_obj.data_handler = update_friends_handler;
	jcs_init_obj.evt_handler = jewelbot_service_evt_handler;
	uint32_t err_code = ble_jcs_init(&m_jcs, &jcs_init_obj);
	APP_ERROR_CHECK(err_code);
}
