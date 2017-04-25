#include <string.h>
#include "app_error.h"
#include "app_timer.h"
#include "fds.h"
#include "nrf_assert.h"
#include "nrf_delay.h"
#include "nrf_error.h"
#include "peer_data_storage.h"
#include "sdk_config.h"
#include "utils.h"

#include "advertising.h"
#include "ble_central_event_handler.h"
#include "friend_data_storage.h"
#include "fsm.h"
#include "jewelbot_types.h"
#include "led_sequence.h"
#include "scan.h"


#define JWB_FRIENDS_INSTANCE_ID 0x0022
#define JWB_FRIENDS_TYPE_ID 0x0021


static bool fl_loaded = false;
static bool m_delete_friends = false;
static bool m_delete_gc = false;
static bool m_redo_write = false;
static friends_list_t *m_friends_list;
bool friends_list_loaded(void) { return fl_loaded; }
void set_friends_list_loaded(void) { fl_loaded = true; }
void set_delete_friends_list(void) {
  m_delete_friends = true;
  set_erase_bonds();
}




void jwb_fds_evt_handler(fds_evt_t const *const p_fds_evt) {
  uint32_t err_code;
//#ifdef DEBUG
//  if (p_fds_evt->result != 0) {
//    NRF_LOG_PRINTF_DEBUG("FLASH STORAGE: FAILURE. %u, cmd: %u\r\n", p_fds_evt->result, p_fds_evt->id);
//  }
//#endif
  switch (p_fds_evt->id) {
  case FDS_EVT_UPDATE:
		if ( ((p_fds_evt->write.file_id) < PDS_FIRST_RESERVED_FILE_ID)
                || ((p_fds_evt->write.record_key) < PDS_FIRST_RESERVED_RECORD_KEY))
			{
				NRF_LOG_PRINTF_DEBUG("FDS_EVT_UPDATE %u\r\n", p_fds_evt->result);
				if (p_fds_evt->result == NRF_SUCCESS) {
					clear_pending_friend();
				}

				//reset_flash_op_status();
				start_advertising(ADV_MODE_NONCONNECTABLE);
				start_scanning();
			}
     break;
  case FDS_EVT_WRITE:
		if ( ((p_fds_evt->write.file_id) < PDS_FIRST_RESERVED_FILE_ID)
                || ((p_fds_evt->write.record_key) < PDS_FIRST_RESERVED_RECORD_KEY))
			{
				NRF_LOG_PRINTF_DEBUG("FDS_EVT_WRITE %u\r\n", p_fds_evt->result);

				//reset_flash_op_status();
				start_advertising(ADV_MODE_NONCONNECTABLE);
				start_scanning();
			}
    break;
  case FDS_EVT_DEL_FILE:
		if ((p_fds_evt->del.record_key == FDS_RECORD_KEY_DIRTY) &&
                    ((p_fds_evt->del.file_id) < PDS_FIRST_RESERVED_RECORD_KEY))
      {
				NRF_LOG_PRINTF_DEBUG("FDS_EVT_DEL_FILE %u\r\n", p_fds_evt->result);
				if (!m_redo_write){
					m_delete_gc = true;
				}
				err_code = fds_gc();
				//NRF_LOG_PRINTF_DEBUG("fds gc ret code = %u\r\n", err_code);
				APP_ERROR_CHECK(err_code);
			}
    break;
  case FDS_EVT_DEL_RECORD:
		if (  ((p_fds_evt->del.file_id) < PDS_FIRST_RESERVED_FILE_ID)
                || ((p_fds_evt->del.record_key) < PDS_FIRST_RESERVED_RECORD_KEY))
      {
				NRF_LOG_PRINTF_DEBUG("FDS_EVT_DEL_RECORD %u\r\n", p_fds_evt->result);
			}
    break;

  case FDS_EVT_GC:
    NRF_LOG_PRINTF_DEBUG("FDS_EVT_GC %u\r\n", p_fds_evt->result);
		if (m_delete_gc) {
			m_delete_gc = false;
			set_create_friends_list();
			initialize_friends_list();
		} else if (m_redo_write) {
			m_redo_write = false;
			create_friends_list_in_flash(m_friends_list);
		} else {
			// Not sure this else is needed
			//NRF_LOG_DEBUG("In else of gc command\r\n");
			//reset_flash_op_status();
			start_advertising(ADV_MODE_NONCONNECTABLE);
			start_scanning();
    }
    break;
	case FDS_EVT_INIT:
		NRF_LOG_PRINTF_DEBUG("FDS_EVT_INIT %u\r\n", p_fds_evt->result);
		if (m_delete_friends) {
			logo_breathe();
			delete_friends_list();
			m_delete_friends = false;
		} else {
			initialize_friends_list();
		}
	break;
  default:
    break;
  }
}

void create_friends_list_in_flash(friends_list_t *friends_list) {
  fds_record_chunk_t chunk;
  fds_record_desc_t friend_list_descriptor;
  fds_find_token_t tok={0};

  bool has_records = false;
	//NRF_LOG_DEBUG("In create friends list\r\n");
  while (fds_record_find(JWB_FRIENDS_TYPE_ID, JWB_FRIENDS_INSTANCE_ID,
                         &friend_list_descriptor, &tok) == NRF_SUCCESS) {
    has_records = true;
  }
  if (!has_records) {
    fds_record_t record;
    record.file_id = JWB_FRIENDS_TYPE_ID;
    record.key = JWB_FRIENDS_INSTANCE_ID;
		//NRF_LOG_DEBUG("in not has records\r\n");
    uint16_t max_length = sizeof(friends_list_t);
    uint16_t length_words = max_length / 4;
    chunk.length_words =
        length_words; // number of 4-byte word chunks of color_friends_t
    uint32_t err_code;
    chunk.p_data = friends_list;
    record.data.p_chunks = &chunk;
    record.data.num_chunks = 1;
    err_code = fds_record_write(&friend_list_descriptor, &record);
    APP_ERROR_CHECK(err_code);
  }
}
void delete_friends_list() {
	//NRF_LOG_DEBUG("Call fds file delete\r\n");
  uint32_t err_code = fds_file_delete(JWB_FRIENDS_TYPE_ID);
	//NRF_LOG_PRINTF_DEBUG("FDS FILE DELETE error %u\r\n", err_code);
}
void save_friends(friends_list_t *friends_list) {
	uint32_t err_code;
	err_code = sd_ble_gap_connect_cancel();
	//NRF_LOG_PRINTF_DEBUG("sd_ble_gap_connect_cancel ret code = %u\r\n", err_code);
	//NRF_LOG_DEBUG("in save friends, stop scan / adv\r\n");
  stop_advertising();
  stop_scanning();
  fds_record_chunk_t chunk;
  fds_record_desc_t friend_list_descriptor;
  fds_record_t record;
  record.file_id = JWB_FRIENDS_TYPE_ID;
  record.key = JWB_FRIENDS_INSTANCE_ID;
  fds_find_token_t tok = {0};

  chunk.p_data = friends_list;
  uint16_t max_length = sizeof(friends_list_t);
  uint16_t length_words = max_length / 4;
  chunk.length_words =
      length_words; // number of 4-byte word chunks of color_friends_t

  record.data.p_chunks = &chunk;
  record.data.num_chunks = 1;
  err_code = fds_record_find(JWB_FRIENDS_TYPE_ID, JWB_FRIENDS_INSTANCE_ID,
                         &friend_list_descriptor, &tok);

  err_code = fds_record_update(&friend_list_descriptor, &record);
  //APP_ERROR_CHECK(err_code);
  //NRF_LOG_PRINTF_DEBUG("fds record update ret code = %u\r\n", err_code);
	if (err_code == FDS_ERR_NO_SPACE_IN_FLASH) {
			m_redo_write = true;
			m_friends_list = friends_list;
      err_code = fds_file_delete(JWB_FRIENDS_TYPE_ID);
      APP_ERROR_CHECK(err_code);
	}
}

void load_friends(friends_list_t *friends_to_load) {
  fds_record_desc_t descriptor;
  fds_find_token_t tok = {0};
  fds_flash_record_t record;
	bool found_nothing = true;

	//NRF_LOG_DEBUG("load friends\r\n");
  while (fds_record_find(JWB_FRIENDS_TYPE_ID, JWB_FRIENDS_INSTANCE_ID, &descriptor, &tok) == NRF_SUCCESS) {
		fds_record_open(&descriptor, &record);
		memcpy(friends_to_load, record.p_data, sizeof(friends_list_t));
		fds_record_close(&descriptor);
		found_nothing = false;
		//NRF_LOG_DEBUG("Found something to load\r\n");
	}

	if (found_nothing) {
		set_create_friends_list();
	}
}
