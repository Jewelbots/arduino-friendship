#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "app_timer.h"
#include "ble.h"
#include "fstorage.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_soc.h"
#include "sdk_config.h"
#include "sdk_errors.h"

#include "ble_central_event_handler.h"
#include "scan.h"
#include "utils.h"

// 0x0168 //225ms
// 0x00A0 // 100 ms
#define SCAN_INTERVAL MSEC_TO_UNITS(160, UNIT_0_625_MS)
// 0x00E0 //140ms
// 0x0050 // 50 ms
#define SCAN_WINDOW MSEC_TO_UNITS(80, UNIT_0_625_MS)

APP_TIMER_DEF(scan_timer_id);
#define SCAN_RESTART_TIMEOUT APP_TIMER_TICKS(20000, APP_TIMER_PRESCALER)

static ble_opt_t m_ble_scan_opt;
static ble_opt_t m_ble_periph_opt;
static ble_opt_t m_ble_central_opt;
static bool m_scan_start_pending = false;
static bool m_scanning = false;
static const ble_gap_scan_params_t m_scan_param = {
    1,    // Active scanning set.
    0,    // Selective scanning not set.
    NULL, // No whitelist provided.
    SCAN_INTERVAL, 
    SCAN_WINDOW, 
    0x0000 // No timeout.
};
const ble_gap_scan_params_t *get_scan_params(void) { return &m_scan_param; }

static void scan_timer_handler(void *p_context) {
  UNUSED_PARAMETER(p_context);
  uint32_t err_code = app_timer_stop(scan_timer_id);
  APP_ERROR_CHECK(err_code);
  start_scanning();
}

void scan_init() {
  app_timer_create(&scan_timer_id, APP_TIMER_MODE_SINGLE_SHOT,
                   scan_timer_handler);
}

void scan_on_sys_evt(uint32_t sys_evt) {
  switch (sys_evt) {
  case NRF_EVT_FLASH_OPERATION_SUCCESS:
  case NRF_EVT_FLASH_OPERATION_ERROR:
    if (m_scan_start_pending) {
      start_scanning();
    }

    break;

  default:
    break;
  }
}
void set_ble_opts(void) {
  m_ble_scan_opt.gap_opt.scan_req_report.enable = 1;
  m_ble_periph_opt.common_opt.conn_bw.role = BLE_GAP_ROLE_PERIPH;
  m_ble_periph_opt.common_opt.conn_bw.conn_bw.conn_bw_rx = BLE_CONN_BW_LOW;
  m_ble_periph_opt.common_opt.conn_bw.conn_bw.conn_bw_tx = BLE_CONN_BW_LOW;

  m_ble_central_opt.common_opt.conn_bw.role = BLE_GAP_ROLE_CENTRAL;
  m_ble_central_opt.common_opt.conn_bw.conn_bw.conn_bw_rx = BLE_CONN_BW_LOW;
  m_ble_central_opt.common_opt.conn_bw.conn_bw.conn_bw_tx = BLE_CONN_BW_LOW;

  uint32_t err_code =
      sd_ble_opt_set(BLE_GAP_OPT_SCAN_REQ_REPORT, &m_ble_scan_opt);
  APP_ERROR_CHECK(err_code);
  err_code = sd_ble_opt_set(BLE_COMMON_OPT_CONN_BW, &m_ble_periph_opt);
  APP_ERROR_CHECK(err_code);
  err_code = sd_ble_opt_set(BLE_COMMON_OPT_CONN_BW, &m_ble_central_opt);
  APP_ERROR_CHECK(err_code);
}

void stop_scanning(void) {
  uint32_t err_code = sd_ble_gap_scan_stop();
	NRF_LOG_PRINTF_DEBUG("STOP SCANNING %u\r\n", err_code);
  if (err_code == NRF_SUCCESS) { 
    m_scanning = false;
    
  }
}

void start_delayed_scanning() {
  app_timer_start(scan_timer_id, SCAN_RESTART_TIMEOUT, NULL);
}

void start_scanning(void) {
  if (flash_access_in_progress()) {
    m_scan_start_pending = true;
    NRF_LOG_DEBUG("SCAN_START_PEND\r\n");
    return;
  }
  stop_scanning();
  uint32_t err_code = sd_ble_gap_scan_start(&m_scan_param);
	NRF_LOG_PRINTF_DEBUG("START SCANNING %u\r\n", err_code);
  if (err_code == NRF_SUCCESS) {
    m_scanning = true;

    m_scan_start_pending = false;
  }
  if (err_code != NRF_SUCCESS) {
    //APP_ERROR_CHECK(err_code);
    m_scan_start_pending = true;
    //app_timer_start(scan_timer_id, SCAN_RESTART_TIMEOUT, NULL);
  }
}
bool is_scanning() { return m_scanning; }

void set_pending_scan_start() { m_scan_start_pending = true; }
