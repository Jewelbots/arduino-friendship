#include <string.h>
#include "app_error.h"
#include "app_error.h"
#include "ble_dis.h"
#include "ble_dis.h"
#include "ble_srv_common.h"
#include "common_defines.h"
#include "jwb_dis.h"
#include "rev.h"

static char *man_name = MANUFACTURER_NAME;
static char *model_number = MODEL_NUMBER;
static char *serial_number = SERIAL_NUMBER;
static char *hw_revision = HARDWARE_REV_VERSION;
static char *fw_revision = FIRMWARE_REV_VERSION;
static char *sw_revision = SOFTWARE_REV_VERSION;
static ble_srv_utf8_str_t manufacturer_name;
static ble_srv_utf8_str_t model_num;
static ble_srv_utf8_str_t serial_num;
static ble_srv_utf8_str_t hw_rev_ver;
static ble_srv_utf8_str_t fw_rev_ver;
static ble_srv_utf8_str_t sw_rev_ver;
static ble_dis_pnp_id_t pnp_id = {
    .vendor_id_source = BLE_DIS_VENDOR_ID_SRC_BLUETOOTH_SIG,
    .vendor_id = HTONS(0x321),      // todo
    .product_id = HTONS(0x001),     // todo add correct product id
    .product_version = HTONS(0x001) // todo add correct version
};
static ble_dis_sys_id_t sys_id = {
    .manufacturer_id = 0x321,
    .organizationally_unique_id = 0x000 // todo: get IEEE OUI
};

static ble_srv_security_mode_t dis_attr_modes = {
    .read_perm = {.sm = 1, .lv = 1}, .write_perm = {.sm = 0, .lv = 0}};

static ble_dis_init_t p_dis_init;

void jwb_dis_init() {
  manufacturer_name.length = sizeof(MANUFACTURER_NAME);
  manufacturer_name.p_str = (uint8_t *)man_name;
  model_num.length = sizeof(MODEL_NUMBER);
  model_num.p_str = (uint8_t *)model_number;
  serial_num.length = sizeof(SERIAL_NUMBER);
  serial_num.p_str = (uint8_t *)serial_number;
  hw_rev_ver.length = sizeof(HARDWARE_REV_VERSION);
  hw_rev_ver.p_str = (uint8_t *)hw_revision;
  fw_rev_ver.length = sizeof(FIRMWARE_REV_VERSION);
  fw_rev_ver.p_str = (uint8_t *)fw_revision;
  sw_rev_ver.length = sizeof(SOFTWARE_REV_VERSION);
  sw_rev_ver.p_str = (uint8_t *)sw_revision;
  memcpy(&(p_dis_init.dis_attr_md), &dis_attr_modes,
         sizeof(ble_srv_security_mode_t));
  memcpy(&(p_dis_init.fw_rev_str), &fw_rev_ver, sizeof(ble_srv_utf8_str_t));
  memcpy(&(p_dis_init.hw_rev_str), &hw_rev_ver, sizeof(ble_srv_utf8_str_t));
  memcpy(&(p_dis_init.sw_rev_str), &sw_rev_ver, sizeof(ble_srv_utf8_str_t));
  memcpy(&(p_dis_init.manufact_name_str), &manufacturer_name,
         sizeof(ble_srv_utf8_str_t));
  memcpy(&(p_dis_init.model_num_str), &model_num, sizeof(ble_srv_utf8_str_t));
  p_dis_init.p_pnp_id = &pnp_id;
  p_dis_init.p_sys_id = &sys_id;
  p_dis_init.p_reg_cert_data_list = NULL;
  memcpy(&(p_dis_init.serial_num_str), &serial_num, sizeof(ble_srv_utf8_str_t));

  uint32_t err_code = ble_dis_init(&p_dis_init);
  APP_ERROR_CHECK(err_code);
}
