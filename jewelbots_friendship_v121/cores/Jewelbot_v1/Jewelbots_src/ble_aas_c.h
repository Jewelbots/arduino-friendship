#ifndef BLE_AAS_C_H_
#define BLE_AAS_C_H_

#include <stdint.h>
#include "ble.h"
#include "ble_gattc.h"
#include "ble_db_discovery.h"
#include "ble_srv_common.h"

#include "jwb_srv_common.h"
#include "messaging.h"

typedef struct ble_aas_c_s ble_aas_c_t;
typedef enum { 
  READ_REQ = 1, 
  WRITE_REQ 
} aas_tx_request_t;

typedef struct {
  uint8_t gattc_value[MAX_MSG_LENGTH];
  ble_gattc_write_params_t
      gattc_params;
} write_params_t;

typedef struct {
  uint16_t
      conn_handle; 
                   
  aas_tx_request_t
      type; 
  union {
    uint16_t read_handle;    
    write_params_t write_req;
  } req;
} tx_message_t;

typedef enum {
  BLE_AAS_C_EVT_DISCOVERY_COMPLETE,
  BLE_AAS_C_EVT_DISCOVERY_FAILED,
  BLE_AAS_C_EVT_AAS_RX_EVT,
  BLE_AAS_C_EVT_ALERT_READ_RESP,
  BLE_AAS_C_EVT_DISCONN_COMPLETE
} ble_aas_c_evt_type_t;

typedef struct {
  uint16_t aas_tx_handle;
  uint16_t aas_rx_handle;
  uint16_t aas_rx_cccd_handle;
} ble_aas_c_handles_t;

typedef struct {
  ble_aas_c_evt_type_t evt_type;
  uint16_t conn_handle;
  uint16_t *p_annotated_alert;
  uint16_t alert_length;
  ble_aas_c_handles_t handles;
} ble_aas_c_evt_t;

typedef void (*ble_aas_c_evt_handler_t)(ble_aas_c_t *p_aas_c,
                                        ble_aas_c_evt_t *p_evt);

struct ble_aas_c_s {
  ble_aas_c_evt_handler_t evt_handler;
  ble_srv_error_handler_t error_handler;
  uint16_t conn_handle;
  ble_uuid_t service_uuid;
  ble_aas_c_handles_t handles;
  ble_gap_addr_t address;
};

typedef struct {
  ble_aas_c_evt_handler_t evt_handler;
  ble_srv_error_handler_t error_handler;
} ble_aas_c_init_t;

void ble_aas_c_on_db_disc_evt(ble_aas_c_t *p_aas_c,
                              const ble_db_discovery_evt_t *p_evt);
uint32_t ble_aas_c_alert_send(ble_aas_c_t *p_aas_c, uint8_t *p_alert,
                              uint16_t length);
uint32_t ble_aas_c_handles_assign(ble_aas_c_t *p_aas_c,
                                  const uint16_t conn_handle,
                                  const ble_aas_c_handles_t *p_peer_handles);
uint32_t ble_aas_c_init(ble_aas_c_t *p_aas_c,
                        const ble_aas_c_init_t *p_aas_c_init);
uint32_t ble_aas_c_notif_source_notif_enable(const ble_aas_c_t *p_aas_c);
void ble_aas_c_on_ble_evt(ble_aas_c_t *p_aas_c, const ble_evt_t *p_ble_evt);

#endif // BLE_AAS_C_H__

