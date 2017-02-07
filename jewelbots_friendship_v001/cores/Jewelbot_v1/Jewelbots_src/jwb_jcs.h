/* Jewelbots Control Service */
/* Copyright (c) 2016 Jewelbots   */
/* Author: George Stocker         */
/* Date: 9/16/2016 				  */
#ifndef JWB_JCS_H_
#define JWB_JCS_H_
#include <stdbool.h>
#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"

#include "jewelbot_types.h"
#include "jwb_srv_common.h"

typedef struct ble_jcs_s ble_jcs_t;

typedef enum {
  BLE_JCS_EVT_NOTIFICATION_ENABLED,
  BLE_JCS_EVT_NOTIFICATION_DISABLED,
  BLE_JCS_EVT_FRIEND_LIST_UPDATED,
  BLE_JCS_EVT_FIRMWARE_UPDATE_REQUESTED
} ble_jcs_evt_type_t;

typedef struct { ble_jcs_evt_type_t evt_type; } ble_jcs_evt_t;

typedef void (*ble_jcs_data_handler_t) (ble_jcs_t * p_jcs, uint8_t * data, uint16_t length);

typedef void (*ble_jcs_evt_handler_t) (ble_jcs_t * p_jcs, ble_jcs_evt_t * p_evt);

typedef struct { 
  ble_jcs_data_handler_t data_handler;
  ble_jcs_evt_handler_t evt_handler;
} ble_jcs_init_t;

struct ble_jcs_s {
  uint16_t service_handle;
  uint16_t conn_handle;
  ble_jcs_evt_handler_t evt_handler;
  ble_jcs_data_handler_t data_handler;
  ble_gatts_char_handles_t tx_handles;
  ble_gatts_char_handles_t rx_handles;
  ble_gatts_char_handles_t dfu_handles;
  uint8_t uuid_type;
  bool is_notification_enabled;
};

uint32_t ble_jcs_init(ble_jcs_t * p_jcs, const ble_jcs_init_t *p_jcs_init);

void ble_jcs_on_ble_evt(ble_jcs_t * p_jcs, ble_evt_t *p_ble_evt);

uint32_t ble_jcs_friends_list_set(ble_jcs_t *p_jcs, friends_list_t * p_list, uint8_t friends_list_length);

#endif
