#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "app_error.h"
#include "app_scheduler.h"
#include "app_timer.h"
#include "app_trace.h"
#include "ble.h"
#include "ble_conn_params.h"
#include "ble_conn_state.h"
#include "ble_db_discovery.h"
#include "ble_gap.h"
#include "ble_gatt.h"
#include "ble_hci.h"
#include "ble_lbs.h"
#include "ble_srv_common.h"
#include "ble_types.h"
#include "dfu_handler.h"
#include "fstorage.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "peer_manager.h"
#include "sdk_config.h"
#include "sdk_errors.h"
#include "softdevice_handler_appsh.h"

#include "advertising.h"
#include "ble_central_event_handler.h"
#include "fsm.h"
#include "jewelbot_service.h"
#include "jwb_jcs.h"
#include "led_sequence.h"
#include "messaging.h"
#include "power_management.h"
#include "scan.h"
#include "utils.h"

#define IS_SRVC_CHANGED_CHARACT_PRESENT 1
#define CENTRAL_LINK_COUNT 2
#define PERIPHERAL_LINK_COUNT 1
#define MAX_CONNECTED_CENTRALS CENTRAL_LINK_COUNT
#define UUID16_SIZE 2
#define MIN_CONNECTION_INTERVAL MSEC_TO_UNITS(40, UNIT_1_25_MS)
#define MAX_CONNECTION_INTERVAL MSEC_TO_UNITS(80, UNIT_1_25_MS)
#define APP_COMPANY_IDENTIFIER 0x123
#define DEVICE_NAME "JWB_011"
// supervision timeout value in decimal > ((1+slave latency)(max conn interval)) / 4
// sup timeout > 7*64/4 = 112, so minimum timeout is 113 as a value or 1,130 MS
#define SUPERVISION_TIMEOUT MSEC_TO_UNITS(2000, UNIT_10_MS)
#define APPL_LOG app_trace_log
#define SLAVE_LATENCY 6
#define ADV_TYPE_LEN 2
#define BEACON_DATA_LEN 21
#define APP_FEATURE_NOT_SUPPORTED            BLE_GATT_STATUS_ATTERR_APP_BEGIN + 2

#define FIRST_CONN_PARAMS_UPDATE_DELAY                                         \
  APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)
#define NEXT_CONN_PARAMS_UPDATE_DELAY                                          \
  APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER)
#define MAX_CONN_PARAMS_UPDATE_COUNT 3
static uint32_t m_current_sys_evt;
uint32_t get_current_sys_evt() { return m_current_sys_evt; }
//static bool friend_not_msg  = false;
#define QUEUED_WRITE_BUFFER_SIZE        182
static uint8_t queued_write_buffer[QUEUED_WRITE_BUFFER_SIZE];
static bool ff_conn_as_central = false;
static bool ff_conn_as_periph = false;
static bool ff_cent_complete = false;
static bool ff_periph_complete = false;

bool get_ff_conn_as_central(void) { return ff_conn_as_central; }
bool get_ff_conn_as_periph(void) { return ff_conn_as_periph; }
bool get_ff_cent_complete(void) { return ff_cent_complete; }
bool get_ff_periph_complete(void) { return ff_periph_complete; }
void set_ff_cent(void) { ff_cent_complete = false; }
void set_ff_periph(void) { ff_periph_complete = false; }
void set_ff_conn_cent(void) { ff_conn_as_central = false; }
void set_ff_conn_periph(void) { ff_conn_as_periph = false; }

static uint16_t m_gap_role = BLE_GAP_ROLE_INVALID;
static ble_gap_conn_params_t m_connection_param = {
    (uint16_t)MIN_CONNECTION_INTERVAL, // 7.5ms
    (uint16_t)MAX_CONNECTION_INTERVAL, // 30ms
    SLAVE_LATENCY,                     // slave latency in number of events
    (uint16_t)SUPERVISION_TIMEOUT      // 430ms
};


static const ble_gap_scan_params_t m_scan_conn_params = {
    1, // could change to 0 for no scan requests?
    0, // selective scanning off
    NULL, // do we have a whitelist . e.g., for messaging
    BLE_GAP_SCAN_INTERVAL_MIN, // 2.5ms
    BLE_GAP_SCAN_WINDOW_MIN,   // 2.5ms
    0x0000                     // no timeout
};

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;
static ble_db_discovery_t m_ble_db_discovery_aas;

bool is_connected() {
  ble_conn_state_status_t conn_state = ble_conn_state_status(m_conn_handle);
  return (conn_state == BLE_CONN_STATUS_CONNECTED);
}
void disconnect() {
  ble_conn_state_status_t conn_state = ble_conn_state_status(m_conn_handle);

  NRF_LOG_PRINTF_DEBUG(
      "WANT TO DISCONNECT, role: %x, m_conn_handle is: %u, conn_state: %u\r\n",
      m_gap_role, m_conn_handle, conn_state);

  uint32_t err_code;
  err_code = sd_ble_gap_disconnect(m_conn_handle,
                                   BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
  NRF_LOG_PRINTF_DEBUG("DISCONNECT: %x\r\n", err_code);
  UNUSED_PARAMETER(err_code);
}


static void on_conn_params_evt(ble_conn_params_evt_t *p_evt) {
  uint32_t err_code;

  if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED) {
    NRF_LOG_DEBUG("BLE_CONN_PARAMS_EVT_FAILED\r\n");
    err_code = sd_ble_gap_disconnect(m_conn_handle,
                                     BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
    m_conn_handle =
        BLE_CONN_HANDLE_INVALID; // todo, see if nordic is also doing it here
    APP_ERROR_CHECK(err_code);
  }
}
static void conn_params_error_handler(uint32_t nrf_error) {
  NRF_LOG_PRINTF_DEBUG("ERROR in CONN PARAMS 0x%04x\r\n", nrf_error);
  APP_ERROR_HANDLER(nrf_error);
}

static void on_ble_peripheral_evt(ble_evt_t *p_ble_evt) {

  uint32_t err_code;
	ble_user_mem_block_t mem_block;

  switch (p_ble_evt->header.evt_id) {

  case BLE_GAP_EVT_CONNECTED: {
    m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

    const ble_gap_addr_t *peer_addr =
        &p_ble_evt->evt.gap_evt.params.connected.peer_addr;
    NRF_LOG_DEBUG("PERIPHERAL: CONNECTED\r\n");

    if (friend_adding_mode()) {
      set_pending_friend(peer_addr);
			break;
    }
    start_advertising(ADV_MODE_NONCONNECTABLE);
    break;
  }

  case BLE_GAP_EVT_DISCONNECTED:
    NRF_LOG_PRINTF_DEBUG("PERIPHERAL: DISCONNECTED: %02x\r\n",
                   p_ble_evt->evt.gap_evt.params.disconnected.reason);
    m_gap_role = BLE_GAP_ROLE_INVALID;
    if (p_ble_evt->evt.gap_evt.conn_handle == m_conn_handle) {
      m_conn_handle = BLE_CONN_HANDLE_INVALID;
    }
		sd_ble_gap_connect_cancel();
    if (get_current_event()->sig == WAITING_FOR_CONNECTEE_SIG) {
      set_jwb_event_signal(CONNECTEE_FOUND_SIG);
			ff_conn_as_periph = true;
			set_ff_index(FF_SENTINEL);
			start_advertising(ADV_MODE_FF_NONCONN);
			start_scanning();
			break;
    }
		NRF_LOG_DEBUG("in peripheral disconnected, starting scan / adv\r\n");
		start_scanning();
		start_advertising(ADV_MODE_NONCONNECTABLE);
    break;

  case BLE_GATTS_EVT_SYS_ATTR_MISSING:
    NRF_LOG_DEBUG("setting sys attr.\r\n");
    err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
    APP_ERROR_CHECK(err_code);
    break;
  case BLE_GAP_EVT_TIMEOUT:
    NRF_LOG_DEBUG("BLE_GAP_EVT_TIMEOUT\r\n");
    break;
	case BLE_EVT_USER_MEM_REQUEST:
			NRF_LOG_DEBUG("ble periph - user mem request\r\n");
			mem_block.len = QUEUED_WRITE_BUFFER_SIZE;
      mem_block.p_mem = &queued_write_buffer[0];
      err_code = sd_ble_user_mem_reply(m_conn_handle, &mem_block);
      APP_ERROR_CHECK(err_code);
      break;
	case BLE_EVT_USER_MEM_RELEASE:
			NRF_LOG_DEBUG("ble periph - user mem release\r\n");
			break;
  default:
    // No implementation needed.
    break;
  }
}

void on_ble_central_evt(ble_evt_t *p_ble_evt) {
  uint32_t err_code;
  ble_gatts_rw_authorize_reply_params_t auth_reply;

  const ble_gap_evt_t *p_gap_evt = &p_ble_evt->evt.gap_evt;
  switch (p_ble_evt->header.evt_id) {

  case BLE_GAP_EVT_CONNECTED: {
    const ble_gap_addr_t *peer_addr = &p_gap_evt->params.connected.peer_addr;
    m_conn_handle = p_gap_evt->conn_handle;
    NRF_LOG_DEBUG("CENTRAL: CONNECTED\r\n");

    if (friend_adding_mode()) {
      set_pending_friend(peer_addr);
			break;
    }
    if (ble_conn_state_n_centrals() == MAX_CONNECTED_CENTRALS) {
    }
    start_scanning();
		start_advertising(ADV_MODE_NONCONNECTABLE);
    break; // BLE_GAP_EVT_CONNECTED
  }

  case BLE_GAP_EVT_DISCONNECTED: {
    NRF_LOG_PRINTF_DEBUG("CENTRAL: DISCONNECTED: %02x\r\n",
                   p_ble_evt->evt.gap_evt.params.disconnected.reason);
    m_gap_role = BLE_GAP_ROLE_INVALID;
    if (p_gap_evt->conn_handle == m_conn_handle) {
      m_conn_handle = BLE_CONN_HANDLE_INVALID;
    }
		sd_ble_gap_connect_cancel();
		if (get_current_event()->sig == WAITING_FOR_CONNECTEE_SIG) {
			if (p_ble_evt->evt.gap_evt.params.disconnected.reason != 0x3E) {
        set_jwb_event_signal(CONNECTEE_FOUND_SIG);
				ff_conn_as_central = true;
				set_ff_index(FF_SENTINEL);
				start_scanning();
				start_advertising(ADV_MODE_FF_NONCONN);
			}
			break;
		}
		NRF_LOG_DEBUG("in central disconnected, starting scan / adv\r\n");
		start_scanning();
		start_advertising(ADV_MODE_NONCONNECTABLE);
    break;
  } // BLE_GAP_EVT_DISCONNECTED
  case BLE_GAP_EVT_ADV_REPORT: {

    data_t adv_data;
		data_t scan_data;
    data_t type_data;
		uint8_t packet_type = p_gap_evt->params.adv_report.scan_rsp;


		if (packet_type == 0) {
			// normal advertising packet
			adv_data.p_data = (uint8_t *)p_gap_evt->params.adv_report.data;
			adv_data.data_len = p_gap_evt->params.adv_report.dlen;
			const ble_gap_addr_t *peer_address =
        &p_gap_evt->params.adv_report.peer_addr;
			err_code = adv_report_parse(BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA,
																	&adv_data, &type_data);
			if (err_code == NRF_SUCCESS) {
				//NRF_LOG_DEBUG("Parse ADV REPORT\r\n");
				if (friend_adding_mode()) {
					if (dest_friend_adding_mode(&type_data)) {
						NRF_LOG_DEBUG("attempt to connect\r\n");
						err_code =
								sd_ble_gap_connect(&p_gap_evt->params.adv_report.peer_addr,
																	 &m_scan_conn_params, &m_connection_param);

						if (err_code == NRF_SUCCESS) {
							//set_jwb_event_signal(CONNECTEE_FOUND_SIG);
						} else {
							//set_jwb_event_signal(MOVE_OUT_OF_RANGE_SIG);
							NRF_LOG_DEBUG("gap connect not NRF SUCCESS\r\n");
						}
					}
				}
				if (dest_jewelbot(&type_data)) {
					bool dest_ff_mode = dest_friend_adding_mode(&type_data);
					set_jwb_found(peer_address, p_gap_evt->params.adv_report.type, dest_ff_mode);
				}
			}
		} else if (packet_type == 1) {
			scan_data.p_data = (uint8_t *)p_gap_evt->params.adv_report.data;
			scan_data.data_len = p_gap_evt->params.adv_report.dlen;
			const ble_gap_addr_t *scan_address =
        &p_gap_evt->params.adv_report.peer_addr;
			err_code = scan_report_parse(BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA,
																	&scan_data, &type_data);
			uint8_t index = 0;
			uint8_t color_index = 9;

			if (err_code == NRF_SUCCESS) {
				//NRF_LOG_DEBUG("scan report parse\r\n");
				if ((dest_jewelbot(&type_data)) && (jwb_friend_found(scan_address, &index)) && (type_data.p_data[MSG_REC_FLAG_INDEX] == 0xBB))  {
					uint8_t scan_index = type_data.p_data[MSG_REC_START_INDEX];
					NRF_LOG_PRINTF_DEBUG("COLOR INDEX IN SCAN RESPONSE: %u\r\n", scan_index);
					uint32_t err_code = get_color_index_for_friend(index, &color_index);
					if ((err_code == NRF_SUCCESS) && (scan_index == color_index)) {
						uint8_t size = type_data.data_len;
						uint8_t msg[50] = {0};
						uint8_t msg_length = size - MSG_REC_START_INDEX - MSG_TAG_LENGTH;
						uint8_t msg_tag[MSG_TAG_LENGTH];
						uint8_t j = 0;

						for (uint8_t i = MSG_REC_START_INDEX; i < size-MSG_TAG_LENGTH; i++) {
							//NRF_LOG_PRINTF_DEBUG("%u ", type_data.p_data[i]);
							msg[j] = type_data.p_data[i];
							j++;
						}
						j = 0;
						for (uint8_t i = size-MSG_TAG_LENGTH; i< size; i++) {
							msg_tag[j] = type_data.p_data[i];
							NRF_LOG_PRINTF_DEBUG("%u ", msg_tag[j]);
							j++;
						}
						NRF_LOG_DEBUG("\r\n");
						if (!(msg_tag_in_queue(msg_tag))) {
							queue_message_for_play(msg, msg_length);
							record_msg_tag(msg_tag);
							set_jwb_event_signal(MSG_RECEIVED_SIG);
						}

					}
					NRF_LOG_DEBUG("Scan report from a friend, but not same color\r\n");
				} else if ((dest_jewelbot(&type_data)) && (jwb_pending_friend_found(scan_address))) {
					if (type_data.p_data[MSG_REC_FLAG_INDEX] == 0x10) {
						//NRF_LOG_DEBUG("scan report from a ff advertise\r\n");
						if (ff_conn_as_central) {
							NRF_LOG_PRINTF_DEBUG("Central: Expected index:  %u, Index in scan report: %u\r\n", get_ff_index(), type_data.p_data[3]);
						 if (type_data.p_data[3] != FF_SENTINEL) {
							 NRF_LOG_DEBUG("Setting central complete to true\r\n");
							 ff_cent_complete = true;
							 ff_conn_as_central = false;
						 }
						} else if (ff_conn_as_periph) {
							NRF_LOG_PRINTF_DEBUG("Peripheral:Index in scan report: %u\r\n", type_data.p_data[3]);
							if (type_data.p_data[3] != FF_SENTINEL) {
								set_current_color(type_data.p_data[3]);
								set_friend_selected();

							}
						}
					} else if ((type_data.p_data[MSG_REC_FLAG_INDEX] == 0xAC) && (ff_conn_as_periph)) {
						if (get_ff_index() != FF_SENTINEL) {
							NRF_LOG_DEBUG("Set peripheral complete to true\r\n");
							ff_periph_complete = true;
							ff_conn_as_periph = false;
						} else {

						}
					}
				}
			}
		}
    break;
  }

  // BLE_GAP_ADV_REPORT

  case BLE_GAP_EVT_TIMEOUT: {
    if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN) {
      NRF_LOG_DEBUG("[APPL]: Connection Request timed out.\r\n");
    }
    NRF_LOG_DEBUG("BLE_GAP_EVT_TIMEOUT\r\n");
  } break; // BLE_GAP_EVT_TIMEOUT

  case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST: {
    // Accept parameters requested by peer.
    err_code = sd_ble_gap_conn_param_update(
        p_gap_evt->conn_handle,
        &p_gap_evt->params.conn_param_update_request.conn_params);
    NRF_LOG_PRINTF_DEBUG("BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST: %u\r\n", err_code);
    APP_ERROR_CHECK(err_code);
  } break; // BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST
  case BLE_EVT_USER_MEM_REQUEST:
			NRF_LOG_DEBUG("ble central - user mem request\r\n");
      err_code = sd_ble_user_mem_reply(m_conn_handle, NULL);
      APP_ERROR_CHECK(err_code);
      break;
	case BLE_EVT_USER_MEM_RELEASE:
			NRF_LOG_DEBUG("ble central - user mem release\r\n");
			break;
  case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
      if (p_ble_evt->evt.gatts_evt.params.authorize_request.type
          != BLE_GATTS_AUTHORIZE_TYPE_INVALID)
      {
          if ((p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.op
               == BLE_GATTS_OP_PREP_WRITE_REQ)
              || (p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.op
                  == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW)
              || (p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.op
                  == BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL))
          {
              if (p_ble_evt->evt.gatts_evt.params.authorize_request.type
                  == BLE_GATTS_AUTHORIZE_TYPE_WRITE)
              {
                  auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
              }
              else
              {
                  auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
              }
              auth_reply.params.write.gatt_status = APP_FEATURE_NOT_SUPPORTED;
              NRF_LOG_DEBUG("AUTH REPLY\r\n");
              err_code = sd_ble_gatts_rw_authorize_reply(m_conn_handle,&auth_reply);
              APP_ERROR_CHECK(err_code);
          }
      }
      break;

  case BLE_GATTS_EVT_SYS_ATTR_MISSING:
  case BLE_GAP_EVT_CONN_SEC_UPDATE:
      //service change update?
      break;

  case BLE_GAP_EVT_AUTH_STATUS:
      // No implementation needed.
      break;

  default:
    break;
  }
}


bool is_connected_to_master_device(uint16_t conn_handle) {
  if (conn_handle != BLE_CONN_HANDLE_INVALID) {
    pm_peer_id_t peer_id;

    pm_peer_id_get(conn_handle, &peer_id);
    return (peer_id != PM_PEER_ID_INVALID);
  }
  return false;
}
/**@brief Function for dispatching a BLE stack event to all modules with a BLE
 * stack event handler.
 *
 * @details This function is called from the scheduler in the main loop after a
 * BLE stack event has
 * been received.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t *p_ble_evt) {

  ble_conn_state_on_ble_evt(p_ble_evt);

  notify_dfu_ble_evt(p_ble_evt);
  //if (get_current_event()->sig == BOND_WITH_MASTER_DEVICE_SIG || get_current_event()->sig == JEWELBOT_CONNECTED_TO_MASTER_SIG || is_connected_to_master_device(p_ble_evt->evt.gap_evt.conn_handle)) {
    pm_on_ble_evt(p_ble_evt);
  //}
  // m_gap_role = ble_conn_state_role(p_ble_evt->evt.gap_evt.conn_handle);
  if (p_ble_evt->header.evt_id != BLE_GAP_EVT_ADV_REPORT) {
    m_gap_role = ble_conn_state_role(p_ble_evt->evt.gap_evt.conn_handle);
  }
  if (m_gap_role == BLE_GAP_ROLE_PERIPH) {
    on_ble_peripheral_evt(p_ble_evt);
    ble_conn_params_on_ble_evt(p_ble_evt);
    ble_jcs_on_ble_evt(get_jcs(), p_ble_evt);
  } else if ((m_gap_role == BLE_GAP_ROLE_CENTRAL) ||
             (p_ble_evt->header.evt_id == BLE_GAP_EVT_ADV_REPORT)) {

    if (p_ble_evt->header.evt_id != BLE_GAP_EVT_DISCONNECTED) {
      on_ble_central_evt(p_ble_evt);
    }
    if (p_ble_evt->evt.gap_evt.conn_handle == m_conn_handle) {
      ble_db_discovery_on_ble_evt(&m_ble_db_discovery_aas, p_ble_evt);
    }
    if (p_ble_evt->header.evt_id == BLE_GAP_EVT_DISCONNECTED) {
      on_ble_central_evt(p_ble_evt);
    }
  }

}

static void sys_evt_dispatch(uint32_t sys_evt) {
  m_current_sys_evt = sys_evt;
  fs_sys_event_handler(sys_evt);
  advertising_on_sys_evt(sys_evt);
  scan_on_sys_evt(sys_evt);
  power_management_on_sys_evt(sys_evt);
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupts.
 */
void ble_stack_init(void) {
  ret_code_t err_code;
  nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;
  // Initialize the SoftDevice handler module.
  // SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, NULL);
  SOFTDEVICE_HANDLER_APPSH_INIT(&clock_lf_cfg, true);
  // SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_RC_250_PPM_TEMP_4000MS_CALIBRATION,
  // true);
  // Enable BLE stack.
  ble_enable_params_t ble_enable_params;
  memset(&ble_enable_params, 0, sizeof(ble_enable_params));
#ifdef S130
  ble_enable_params.gatts_enable_params.attr_tab_size =
      BLE_GATTS_ATTR_TAB_SIZE_DEFAULT;
#endif
  ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
  err_code = softdevice_enable_get_default_config(
      CENTRAL_LINK_COUNT, PERIPHERAL_LINK_COUNT, &ble_enable_params);
  APP_ERROR_CHECK(err_code);
  CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT, PERIPHERAL_LINK_COUNT);
  ble_enable_params.common_enable_params.vs_uuid_count = 3;
  ble_enable_params.common_enable_params.p_conn_bw_counts->tx_counts.mid_count =
      2;
  ble_enable_params.common_enable_params.p_conn_bw_counts->rx_counts.mid_count =
      2;
  err_code = softdevice_enable(&ble_enable_params);
  APP_ERROR_CHECK(err_code);
  // Register with the SoftDevice handler module for BLE events.
  err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
  APP_ERROR_CHECK(err_code);

  // Register with the SoftDevice handler module for System events.
  err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
  APP_ERROR_CHECK(err_code);
}
void conn_params_init(void) {
  uint32_t err_code;
  ble_conn_params_init_t cp_init;

  memset(&cp_init, 0, sizeof(cp_init));

  cp_init.p_conn_params = &m_connection_param;
  cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
  cp_init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;
  cp_init.max_conn_params_update_count = MAX_CONN_PARAMS_UPDATE_COUNT;
  cp_init.start_on_notify_cccd_handle = BLE_CONN_HANDLE_INVALID;
  cp_init.disconnect_on_fail = false;
  cp_init.evt_handler = on_conn_params_evt;
  cp_init.error_handler = conn_params_error_handler;

  err_code = ble_conn_params_init(&cp_init);
  APP_ERROR_CHECK(err_code);
}

void gap_params_init(void) {
  uint32_t err_code;
  ble_gap_conn_params_t gap_conn_params;
  ble_gap_conn_sec_mode_t sec_mode;

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
  err_code = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)DEVICE_NAME,
                                        strlen(DEVICE_NAME));
  APP_ERROR_CHECK(err_code);

  memset(&gap_conn_params, 0, sizeof(gap_conn_params));

  gap_conn_params.min_conn_interval = MIN_CONNECTION_INTERVAL;
  gap_conn_params.max_conn_interval = MAX_CONNECTION_INTERVAL;
  gap_conn_params.slave_latency = SLAVE_LATENCY;
  gap_conn_params.conn_sup_timeout = SUPERVISION_TIMEOUT;

  err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
  APP_ERROR_CHECK(err_code);

}
