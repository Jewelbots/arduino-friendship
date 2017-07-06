#include "app_error.h"
#include "ble_db_discovery.h"
#include "nrf_log.h"



static void db_disc_handler(ble_db_discovery_evt_t *p_evt) {
//  ble_aas_c_on_db_disc_evt(get_aas_c(), p_evt);
  NRF_LOG_PRINTF_DEBUG("DISC EVT: %u\r\n", p_evt->evt_type);
}

void db_discovery_init(void) {
  ret_code_t err_code = ble_db_discovery_init(db_disc_handler);
  APP_ERROR_CHECK(err_code);
}
