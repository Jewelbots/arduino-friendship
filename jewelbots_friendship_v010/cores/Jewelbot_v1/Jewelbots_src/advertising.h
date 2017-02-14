#ifndef ADVERTISING_H_
#define ADVERTISING_H_
#include <stdbool.h>
#include <stdint.h>

#define FF_SENTINEL 0x09 //higher than allowed color index
typedef struct {
  uint8_t *p_data;   /**< Pointer to data. */
  uint16_t data_len; /**< Length of data. */
} data_t;

typedef enum {
  ADV_MODE_OFF,
  ADV_MODE_CONNECTABLE,
  ADV_MODE_NONCONNECTABLE,
	ADV_MODE_MSG_NONCONN,
	ADV_MODE_FF_NONCONN,
} advertising_mode_t;

advertising_mode_t get_advertising_mode(void);
void init_advertising_module(void);
void advertising_on_sys_evt(uint32_t sys_evt);
void start_delayed_advertising(advertising_mode_t mode);
void start_advertising(advertising_mode_t mode);
void stop_advertising(void);
uint32_t adv_report_parse(uint8_t type, data_t *p_advdata, data_t *p_typedata);
uint32_t scan_report_parse(uint8_t type, data_t *p_advdata, data_t *p_typedata);
bool is_advertising(void);
void set_pending_adv_start(void);
void set_ff_index(uint8_t ff_index_in);
uint8_t get_ff_index(void);
#endif
