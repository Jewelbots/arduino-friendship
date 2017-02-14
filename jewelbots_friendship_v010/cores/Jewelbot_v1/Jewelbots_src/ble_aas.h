/* Modified code Copyright (c) 2016 Jewelbots. */

#ifndef BLE_AAS_H__
#define BLE_AAS_H__

#include <stdbool.h>
#include <stdint.h>

#include "ble.h"
#include "jwb_srv_common.h"

typedef struct ble_aas_s ble_aas_t;

typedef enum { BLE_AAS_EVT_NOTIFICATION_ENABLED } ble_aas_evt_type_t;
typedef struct { ble_aas_evt_type_t evt_type; } ble_aas_evt_t;

typedef void (*ble_aas_data_handler_t)(ble_aas_t *p_aas, uint8_t *data,
                                       uint8_t length);
typedef void (*ble_aas_evt_handler_t)(ble_aas_t *p_aas, ble_aas_evt_t *p_evt);

typedef struct {
  ble_aas_evt_handler_t evt_handler;
  ble_aas_data_handler_t data_handler;
} ble_aas_init_t;

struct ble_aas_s {
  ble_gap_addr_t address;
  uint16_t service_handle;
  uint16_t conn_handle;
  ble_aas_evt_handler_t evt_handler;
  ble_aas_data_handler_t data_handler;
  ble_gatts_char_handles_t tx_handles;
  ble_gatts_char_handles_t rx_handles;
  uint8_t uuid_type;
  bool is_notification_enabled;
};

uint32_t ble_aas_init(ble_aas_t *p_aas, const ble_aas_init_t *p_aas_init);

void ble_aas_on_ble_evt(ble_aas_t *p_aas, ble_evt_t *p_ble_evt);

uint32_t ble_aas_alert_set(ble_aas_t *p_aas, uint8_t *p_alert,
                           uint8_t *p_alert_length);

#endif // BLE_AAS_H__

/** @} */
