#ifndef BLE_CENTRAL_EVENT_HANDLER_H
#define BLE_CENTRAL_EVENT_HANDLER_H
#include "ble.h"
#include "ble_gap.h"
#include "ble_srv_common.h"
#include "ble_types.h"
#include <stdbool.h>
#include <stdint.h>
#include "advertising.h"

void ble_stack_init(void);
void gap_params_init(void);
uint32_t get_current_sys_evt(void);
void on_ble_central_evt(ble_evt_t *p_ble_evt);
void disconnect(void);
uint32_t connect_to_check_for_message(const ble_gap_addr_t *peer_addr);
bool is_connected(void);
void conn_params_init(void);
//void reset_flash_op_status(void);
bool is_connected_to_master_device(uint16_t conn_handle);
bool get_ff_conn_as_central(void);
bool get_ff_conn_as_periph(void);
bool get_ff_cent_complete(void);
bool get_ff_periph_complete(void);
void set_ff_cent(void);
void set_ff_periph(void);
void set_ff_conn_cent(void);
void set_ff_conn_periph(void);

typedef struct {
  uint16_t manuf_id;       /** manufacturer specific identifier*/
  uint8_t beacon_dev_type; /** manufacturer data type*/
  ble_uuid128_t uuid;      /** 128 uuid*/
  uint16_t major;          /** beacon Major*/
  uint16_t minor;          /** beacon Minor*/
  uint16_t rssi;           /** beacon Rssi */
} adv_data_t;

#define NRF_CLOCK_LFCLKSRC                                                     \
  {                                                                            \
    .source = NRF_CLOCK_LF_SRC_RC, .rc_ctiv = 16, .rc_temp_ctiv = 2,           \
    .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_250_PPM                        \
  }

#endif
