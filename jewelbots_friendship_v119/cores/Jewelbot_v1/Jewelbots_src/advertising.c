#include <stdint.h>
#include "app_error.h"
#include "app_timer.h"
#include "ble.h"
#include "ble_advdata.h"
#include "ble_lbs.h"
#include "ble_srv_common.h"
#include "led_sequence.h"
#include "messaging.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_soc.h"
#include "peer_manager.h"
#include "sdk_config.h"
#include "sdk_errors.h"

#include "advertising.h"
#include "ble_central_event_handler.h"
#include "fsm.h"
#include "utils.h"

#define MANUFACTURER_NAME "Jewelbots"

#define APP_COMPANY_IDENTIFIER 0x213
#define ADV_RESTART_TIMEOUT APP_TIMER_TICKS(20000, APP_TIMER_PRESCALER)
#define APP_CFG_NON_CONN_ADV_TIMEOUT 0

#define NUM_CONN_APPS 8

#define CONN_ADV_INT 80
#define NONCONN_ADV_INT 160

static bool m_advertising_start_pending = false;
static bool m_advertising = false;
static advertising_mode_t m_current_advertising_mode = ADV_MODE_OFF;
static uint8_t m_ff_index = FF_SENTINEL;

static ble_gap_whitelist_t m_whitelist;
static ble_gap_addr_t * mp_whitelist_addr[NUM_CONN_APPS];
static ble_gap_irk_t * mp_whitelist_irk[NUM_CONN_APPS];


void set_ff_index(uint8_t ff_index_in) {
	m_ff_index = ff_index_in;
}
uint8_t get_ff_index(void) { return m_ff_index; }


static ble_gap_adv_params_t m_adv_params;
static ble_advdata_t m_adv_data;
APP_TIMER_DEF(advertising_timer_id);
static ble_uuid_t m_adv_uuids[] = {
  {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}};

static uint8_t custom_data_data[2];
static ble_advdata_manuf_data_t manufacturer_specific_data;
static uint8_t m_manuf_data_array[BLE_GAP_ADV_MAX_SIZE];

static void get_manufact_specific_data(
    bool friend_finding, ble_advdata_manuf_data_t *manufacturer_specific_data) {
  if (friend_finding == true) {
    custom_data_data[0] = 0x10; // is in friend finding mode
  } else {
    custom_data_data[0] = 0xBB; // not in friend finding mode
  }
  custom_data_data[1] = 0xED;

  manufacturer_specific_data->company_identifier = 0x2103;
  manufacturer_specific_data->data.p_data = custom_data_data;
  manufacturer_specific_data->data.size = sizeof(custom_data_data);
}

static void get_ff_manufact_specific_data(ble_advdata_manuf_data_t *manufacturer_specific_data) {
  custom_data_data[0] = 0xCE; // not in friend finding mode
  custom_data_data[1] = 0xED;

  manufacturer_specific_data->company_identifier = 0x2103;
  manufacturer_specific_data->data.p_data = custom_data_data;
  manufacturer_specific_data->data.size = sizeof(custom_data_data);
}

static void advertising_timer_handler(void *p_context) {
  advertising_mode_t mode = (*(advertising_mode_t *)p_context);
  uint32_t err_code = app_timer_stop(advertising_timer_id);
  APP_ERROR_CHECK(err_code);
  start_advertising(mode);
}

void init_advertising_module() {
  app_timer_create(&advertising_timer_id, APP_TIMER_MODE_SINGLE_SHOT,
                   advertising_timer_handler);

  memset(&manufacturer_specific_data, 0, sizeof(manufacturer_specific_data));
  memset(m_manuf_data_array, 0, sizeof(uint8_t[BLE_GAP_ADV_MAX_SIZE]));
  memset(custom_data_data, 0, sizeof(uint8_t[2]));
  memset(&m_adv_params, 0, sizeof(m_adv_params));
  memset(&m_adv_data, 0, sizeof(m_adv_data));

  m_adv_data.name_type = BLE_ADVDATA_FULL_NAME;
  m_adv_data.short_name_len = 6;
  m_adv_data.include_appearance = false;
  m_adv_data.include_ble_device_addr = true;
  //m_adv_data.le_role = BLE_ADVDATA_ROLE_NOT_PRESENT;
  m_adv_data.p_tk_value = NULL;
  m_adv_data.p_sec_mgr_oob_flags = NULL;
  //m_adv_data.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
  m_adv_data.uuids_more_available.uuid_cnt = 0; // not really true.
  m_adv_data.uuids_complete.uuid_cnt = 0;       // not true
  m_adv_data.uuids_solicited.uuid_cnt = 0;
  m_adv_data.p_slave_conn_int = NULL;
  m_adv_data.service_data_count = 0;

//  get_manufact_specific_data(false, &manufacturer_specific_data);
//  m_adv_data.p_manuf_specific_data = &manufacturer_specific_data;
//  m_adv_data.p_manuf_specific_data->company_identifier =
//      manufacturer_specific_data.company_identifier;
//  m_adv_data.p_manuf_specific_data->data.size =
//      manufacturer_specific_data.data.size;

  //m_adv_params.fp = BLE_GAP_ADV_FP_ANY;
  m_adv_params.p_peer_addr = NULL;
  //m_adv_params.p_whitelist = NULL;
  m_adv_params.timeout = BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED;
}
static void advertising_init(bool friend_finding) {

  m_adv_data.le_role = BLE_ADVDATA_ROLE_BOTH_CENTRAL_PREFERRED;

  get_manufact_specific_data(friend_finding, &manufacturer_specific_data);
  m_adv_data.p_manuf_specific_data = &manufacturer_specific_data;
  m_adv_data.p_manuf_specific_data->company_identifier =
      manufacturer_specific_data.company_identifier;
  m_adv_data.p_manuf_specific_data->data.size =
      manufacturer_specific_data.data.size;
	m_adv_data.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

	m_adv_params.fp = BLE_GAP_ADV_FP_ANY;
	m_adv_params.p_whitelist = NULL;
  m_adv_params.type = BLE_GAP_ADV_TYPE_ADV_IND;

  m_adv_params.interval = MSEC_TO_UNITS(CONN_ADV_INT, UNIT_0_625_MS);
}

// Now this mode is connectable but filtered by whitelist from peer manager
// Only connectable to the app, after the initial pairing process
static void non_connectable_advertising_init() {



  get_manufact_specific_data(false, &manufacturer_specific_data);
  m_adv_data.p_manuf_specific_data = &manufacturer_specific_data;
  m_adv_data.p_manuf_specific_data->company_identifier =
      manufacturer_specific_data.company_identifier;
  m_adv_data.p_manuf_specific_data->data.size =
      manufacturer_specific_data.data.size;
  m_adv_data.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
  m_adv_data.uuids_complete.p_uuids = m_adv_uuids;
	//m_adv_data.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

	// create whitelist
	ret_code_t err_code;
	// Storage for the whitelist.
	ble_gap_irk_t * irks[NUM_CONN_APPS];
  ble_gap_addr_t * addrs[NUM_CONN_APPS];
  ble_gap_whitelist_t whitelist = {.pp_irks = irks, .pp_addrs = addrs};

	// Construct a list of peer IDs.
	pm_peer_id_t peer_ids[NUM_CONN_APPS] = {PM_PEER_ID_INVALID};
	pm_peer_id_t peer_id = pm_next_peer_id_get(PM_PEER_ID_INVALID);
	uint32_t n_peer_ids = 0;
	while((peer_id != PM_PEER_ID_INVALID) && (n_peer_ids < NUM_CONN_APPS))
	{
			peer_ids[n_peer_ids++] = peer_id;
			peer_id = pm_next_peer_id_get(peer_id);
	}
	// Create the whitelist.
	err_code = pm_whitelist_create(peer_ids, n_peer_ids, &whitelist);
	APP_ERROR_CHECK(err_code);

	ble_gap_whitelist_t * p_whitelist = &whitelist;
	m_whitelist.addr_count = p_whitelist->addr_count;
	m_whitelist.irk_count  = p_whitelist->irk_count;
	uint32_t i;
	for (i = 0; i < m_whitelist.irk_count; i++)
	{
			mp_whitelist_irk[i] = p_whitelist->pp_irks[i];
	}

	for (i = 0; i < m_whitelist.addr_count; i++)
	{
			mp_whitelist_addr[i] = p_whitelist->pp_addrs[i];
	}
	m_whitelist.pp_addrs = mp_whitelist_addr;
  m_whitelist.pp_irks  = mp_whitelist_irk;

	NRF_LOG_PRINTF_DEBUG("n_peer_ids %u, num addr %u, num irk %u\r\n", n_peer_ids, m_whitelist.addr_count, m_whitelist.irk_count);
//	for (uint8_t i=0; i<m_whitelist.addr_count; i++){
//		uint8_t str[50] = {0};
//		ble_address_to_string_convert(**m_whitelist.pp_addrs, str);
//    NRF_LOG_PRINTF_DEBUG("ADDR:%s \r\n", str);
//	}
//	for (uint8_t i=0; i<whitelist.irk_count; i++){
//		NRF_LOG_PRINTF_DEBUG("addr = %u\r\n", *m_whitelist.pp_irks);
//	}
	NRF_LOG_PRINTF_DEBUG("pm whitelist create %u\r\n", err_code);


	if ((n_peer_ids != 0) && (!(is_connected()))) {
		NRF_LOG_DEBUG("n_peer_ids > 0\r\n");
		m_adv_data.le_role = BLE_ADVDATA_ROLE_BOTH_PERIPH_PREFERRED;
		m_adv_data.flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;
		m_adv_params.p_whitelist = &m_whitelist;
		m_adv_params.fp = BLE_GAP_ADV_FP_FILTER_CONNREQ;
		m_adv_params.type = BLE_GAP_ADV_TYPE_ADV_IND;
	} else {
		NRF_LOG_DEBUG("n_peer_ids = 0\r\n");
		m_adv_data.le_role = BLE_ADVDATA_ROLE_NOT_PRESENT;
		m_adv_data.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
		m_adv_params.p_whitelist = NULL;
		m_adv_params.fp = BLE_GAP_ADV_FP_ANY;
		m_adv_params.type = BLE_GAP_ADV_TYPE_ADV_SCAN_IND;
	}

  m_adv_params.interval = MSEC_TO_UNITS(NONCONN_ADV_INT, UNIT_0_625_MS);
}

static void msg_non_conn_advertising_init() {

  m_adv_data.le_role = BLE_ADVDATA_ROLE_NOT_PRESENT;
  get_manufact_specific_data(false, &manufacturer_specific_data);
  m_adv_data.p_manuf_specific_data = &manufacturer_specific_data;
  m_adv_data.p_manuf_specific_data->company_identifier =
      manufacturer_specific_data.company_identifier;
  m_adv_data.p_manuf_specific_data->data.size =
      manufacturer_specific_data.data.size;
	m_adv_data.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

	m_adv_params.fp = BLE_GAP_ADV_FP_ANY;
	m_adv_params.p_whitelist = NULL;
  m_adv_params.type = BLE_GAP_ADV_TYPE_ADV_SCAN_IND;
  m_adv_params.interval = MSEC_TO_UNITS(NONCONN_ADV_INT, UNIT_0_625_MS);
}

static void ff_non_conn_advertising_init() {

  m_adv_data.le_role = BLE_ADVDATA_ROLE_NOT_PRESENT;
  get_ff_manufact_specific_data(&manufacturer_specific_data);
  m_adv_data.p_manuf_specific_data = &manufacturer_specific_data;
  m_adv_data.p_manuf_specific_data->company_identifier =
      manufacturer_specific_data.company_identifier;
  m_adv_data.p_manuf_specific_data->data.size =
      manufacturer_specific_data.data.size;
	m_adv_data.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

	m_adv_params.fp = BLE_GAP_ADV_FP_ANY;
	m_adv_params.p_whitelist = NULL;
  m_adv_params.type = BLE_GAP_ADV_TYPE_ADV_SCAN_IND;
  m_adv_params.interval = MSEC_TO_UNITS(NONCONN_ADV_INT, UNIT_0_625_MS);
}

void stop_advertising() {
  //if (m_advertising) {
    uint32_t err_code;
    err_code = sd_ble_gap_adv_stop();
		NRF_LOG_PRINTF_DEBUG("STOP ADVERTISING: %u\r\n", err_code);
    if (err_code == NRF_SUCCESS) {
      m_advertising = false;
      m_current_advertising_mode = ADV_MODE_OFF;

    }
  //}
}
advertising_mode_t get_advertising_mode() { return m_current_advertising_mode; }

static void non_connectable_advertising_start() {
  if (flash_access_in_progress()) {
    m_advertising_start_pending = true;
    m_current_advertising_mode = ADV_MODE_NONCONNECTABLE;
    NRF_LOG_DEBUG("NON_CONN_ADV_START_PEND\r\n");
    return;
  }

  stop_advertising();

  uint32_t err_code;
  non_connectable_advertising_init();

	uint8_t ff_data[2] = {0};

	// Prepare the scan response packet
	ble_advdata_manuf_data_t manuf_data_response;
	ff_data[0] = 0xAC;
	ff_data[1] = FF_SENTINEL;

	manuf_data_response.company_identifier = 0x2103;
	manuf_data_response.data.p_data = ff_data;
	manuf_data_response.data.size = sizeof(ff_data);

	ble_advdata_t   advdata_response;
	memset(&advdata_response, 0, sizeof(advdata_response));
	// Populate the scan response packet
	advdata_response.name_type               = BLE_ADVDATA_NO_NAME;
	advdata_response.p_manuf_specific_data   = &manuf_data_response;
  err_code = ble_advdata_set(&m_adv_data, &advdata_response);
  APP_ERROR_CHECK(err_code);
	NRF_LOG_PRINTF_DEBUG("ble adv data set %u\r\n", err_code);
  err_code = sd_ble_gap_adv_start(&m_adv_params);
	NRF_LOG_PRINTF_DEBUG("START NON CON ADV %u\r\n", err_code);
  if (err_code == NRF_SUCCESS) { //  || err_code == NRF_ERROR_INVALID_STATE) {
    m_advertising = true;
    m_current_advertising_mode = ADV_MODE_NONCONNECTABLE;
    m_advertising_start_pending = false;

  }
  if (err_code != NRF_SUCCESS) {
    //APP_ERROR_CHECK(err_code);
    m_advertising_start_pending = true;
    //NRF_LOG_DEBUG("Non-Conn Adv Start Failed\r\n");
  }
  // APP_ERROR_CHECK(err_code);
}

static void advertising_start(bool friend_finding) {
  if (flash_access_in_progress()) {
    m_advertising_start_pending = true;
    m_current_advertising_mode = ADV_MODE_CONNECTABLE;
    NRF_LOG_DEBUG("CONN_ADV_START_PEND\r\n");
    return;
  }
  stop_advertising();
  advertising_init(friend_finding);
  uint32_t err_code;
	ble_advdata_manuf_data_t manuf_data_response;
	manuf_data_response.company_identifier = NULL;
	manuf_data_response.data.p_data = NULL;
	manuf_data_response.data.size = 0;

	ble_advdata_t   advdata_response;
	memset(&advdata_response, 0, sizeof(advdata_response));
	// Populate the scan response packet
	advdata_response.name_type               = BLE_ADVDATA_NO_NAME;
	advdata_response.p_manuf_specific_data   = &manuf_data_response;
  err_code = ble_advdata_set(&m_adv_data, &advdata_response);
  APP_ERROR_CHECK(err_code);
  err_code = sd_ble_gap_adv_start(&m_adv_params);
	NRF_LOG_PRINTF_DEBUG("START CONN ADV %u\r\n", err_code);
  if (err_code == NRF_SUCCESS) { //  || err_code == NRF_ERROR_INVALID_STATE) {
    m_advertising = true;
    m_current_advertising_mode = ADV_MODE_CONNECTABLE;
    m_advertising_start_pending = false;

  }
  if (err_code != NRF_SUCCESS) {
    //APP_ERROR_CHECK(err_code);
    m_advertising_start_pending = true;
    //NRF_LOG_DEBUG("Conn Adv Start Failed\r\n");
  }
  // APP_ERROR_CHECK(err_code);
}

static void msg_non_conn_advertising_start() {
  if (flash_access_in_progress()) {
    m_advertising_start_pending = true;
    m_current_advertising_mode = ADV_MODE_MSG_NONCONN;
    NRF_LOG_DEBUG("MSG_NON_CONN_ADV_START_PEND\r\n");
    return;
  }

  stop_advertising();
	NRF_LOG_DEBUG("Start of msg non conn adv\r\n");
  uint32_t err_code;
  msg_non_conn_advertising_init();
	uint8_t color_index = get_current_msg_color_cycle();
	uint8_t msg[50] = {0};
  uint8_t size = 0;
	msg_queue_pop(color_index, msg, &size);

	// Prepare the scan response packet
	ble_advdata_manuf_data_t manuf_data_response;
	uint8_t message[50];
	// Insert scan response type flag
	message[0] = 0xBB;
	message[COLOR_INDEX_2] = color_index;
	if (size > MAX_MSG_SIZE) {
    size = MAX_MSG_SIZE;
  }
	uint8_t j = 0;
  for (uint8_t i = MSG_START_INDEX_2; i <= size + 1; i++) {
    message[i] = msg[j];
    j++;
  }

	uint8_t msg_tag[MSG_TAG_LENGTH];
	uint8_t num_rand_bytes_available = 0;
	err_code = sd_rand_application_bytes_available_get(&num_rand_bytes_available);
	APP_ERROR_CHECK(err_code);
	if(num_rand_bytes_available > 0) {
		err_code = sd_rand_application_vector_get(msg_tag, MSG_TAG_LENGTH);
    APP_ERROR_CHECK(err_code);
  }
	j = j + MSG_PADDING;
	for (uint8_t i = 0; i < MSG_TAG_LENGTH; i++) {
		message[j] = msg_tag[i];
		j++;
	}
	uint8_t total_length = size + MSG_PADDING + MSG_TAG_LENGTH;
	manuf_data_response.company_identifier = 0x2103;
	manuf_data_response.data.p_data = message;
	manuf_data_response.data.size = total_length;

	NRF_LOG_DEBUG("message into scan response: ");
	for (uint8_t i = 0; i < total_length; i++) {
		NRF_LOG_PRINTF_DEBUG("%u ", message[i]);
	}
	NRF_LOG_DEBUG("\r\n");

	ble_advdata_t   advdata_response;
	memset(&advdata_response, 0, sizeof(advdata_response));
	// Populate the scan response packet
	advdata_response.name_type               = BLE_ADVDATA_NO_NAME;
	advdata_response.p_manuf_specific_data   = &manuf_data_response;

  err_code = ble_advdata_set(&m_adv_data, &advdata_response);
  APP_ERROR_CHECK(err_code);
  err_code = sd_ble_gap_adv_start(&m_adv_params);
	NRF_LOG_PRINTF_DEBUG("START NON CON MSG ADV %u\r\n", err_code);
  if (err_code == NRF_SUCCESS) { //  || err_code == NRF_ERROR_INVALID_STATE) {
    m_advertising = true;
    m_current_advertising_mode = ADV_MODE_MSG_NONCONN;
    m_advertising_start_pending = false;

  }
  if (err_code != NRF_SUCCESS) {
    //APP_ERROR_CHECK(err_code);
    m_advertising_start_pending = true;
    //NRF_LOG_DEBUG("Non-Conn Adv Start Failed\r\n");
  }
  // APP_ERROR_CHECK(err_code);
}

static void ff_non_conn_advertising_start() {
  if (flash_access_in_progress()) {
    m_advertising_start_pending = true;
    m_current_advertising_mode = ADV_MODE_MSG_NONCONN;
    NRF_LOG_DEBUG("FF_NON_CONN_ADV_START_PEND\r\n");
    return;
  }

  stop_advertising();
	NRF_LOG_DEBUG("Start of ff non conn adv\r\n");
  uint32_t err_code;
  ff_non_conn_advertising_init();
	uint8_t ff_data[2] = {0};

	// Prepare the scan response packet
	ble_advdata_manuf_data_t manuf_data_response;
	ff_data[0] = 0x10;
	ff_data[1] = m_ff_index;

	manuf_data_response.company_identifier = 0x2103;
	manuf_data_response.data.p_data = ff_data;
	manuf_data_response.data.size = sizeof(ff_data);

	NRF_LOG_DEBUG("FF into scan response: ");
	for (uint8_t i = 0; i < 2; i++) {
		NRF_LOG_PRINTF_DEBUG("%u ", ff_data[i]);
	}
	NRF_LOG_DEBUG("\r\n");

	ble_advdata_t   advdata_response;
	memset(&advdata_response, 0, sizeof(advdata_response));
	// Populate the scan response packet
	advdata_response.name_type               = BLE_ADVDATA_NO_NAME;
	advdata_response.p_manuf_specific_data   = &manuf_data_response;

  err_code = ble_advdata_set(&m_adv_data, &advdata_response);
  APP_ERROR_CHECK(err_code);
  err_code = sd_ble_gap_adv_start(&m_adv_params);
	NRF_LOG_PRINTF_DEBUG("START NON CON FF ADV %u\r\n", err_code);
  if (err_code == NRF_SUCCESS) { //  || err_code == NRF_ERROR_INVALID_STATE) {
    m_advertising = true;
    m_current_advertising_mode = ADV_MODE_FF_NONCONN;
    m_advertising_start_pending = false;

  }
  if (err_code != NRF_SUCCESS) {
    //APP_ERROR_CHECK(err_code);
    m_advertising_start_pending = true;
    //NRF_LOG_DEBUG("Non-Conn Adv Start Failed\r\n");
  }
  // APP_ERROR_CHECK(err_code);
}

void start_delayed_advertising(advertising_mode_t mode) {
  NRF_LOG_DEBUG("DELAYED_ADV_START\r\n");
  app_timer_start(advertising_timer_id, ADV_RESTART_TIMEOUT, (void *)mode);
}
void start_advertising(advertising_mode_t mode) {
  if (mode == ADV_MODE_CONNECTABLE) {
    advertising_start(friend_adding_mode());
  } else if (mode == ADV_MODE_NONCONNECTABLE) {
    non_connectable_advertising_start();
  } else if (mode == ADV_MODE_MSG_NONCONN) {
		msg_non_conn_advertising_start();
	} else if (mode == ADV_MODE_FF_NONCONN) {
		ff_non_conn_advertising_start();
	}
}

void advertising_on_sys_evt(uint32_t sys_evt) {
  switch (sys_evt) {
  case NRF_EVT_FLASH_OPERATION_SUCCESS:
  case NRF_EVT_FLASH_OPERATION_ERROR:
    if (m_advertising_start_pending) {
      start_advertising(m_current_advertising_mode);
    }

    break;

  default:
    break;
  }
}

uint32_t adv_report_parse(uint8_t type, data_t *p_advdata, data_t *p_typedata) {
  uint32_t index = 0;
  uint8_t *p_data;

  p_data = p_advdata->p_data;

  while (index < p_advdata->data_len) {
    uint8_t field_length = p_data[index];
    uint8_t field_type = p_data[index + 1];
    if (field_type == type) {
      p_typedata->p_data = &p_data[index + 2];
      p_typedata->data_len = field_length - 1;
      return NRF_SUCCESS;
    }
    index += field_length + 1;
  }
  return NRF_ERROR_NOT_FOUND;
}

uint32_t scan_report_parse(uint8_t type, data_t *p_advdata, data_t *p_typedata) {
  uint32_t index = 0;
  uint8_t *p_data;

  p_data = p_advdata->p_data;

  while (index < p_advdata->data_len) {
    uint8_t field_length = p_data[index];
    uint8_t field_type = p_data[index + 1];
    if (field_type == type) {
      p_typedata->p_data = &p_data[index + 2];
      p_typedata->data_len = field_length - 1;
      return NRF_SUCCESS;
    }
    index += field_length + 1;
  }
  return NRF_ERROR_NOT_FOUND;
}
bool is_advertising() { return m_advertising; }
void set_pending_adv_start() { m_advertising_start_pending = true; }
