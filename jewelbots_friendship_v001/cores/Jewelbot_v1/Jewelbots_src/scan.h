#ifndef SCAN_H_
#define SCAN_H_
#include <stdbool.h>
#include "ble_gap.h"
const ble_gap_scan_params_t *get_scan_params(void);
void start_scanning(void);
void start_delayed_scanning(void);
void stop_scanning(void);
void scan_on_sys_evt(uint32_t sys_evt);
void set_ble_opts(void);
bool is_scanning(void);
void scan_init(void);
void set_pending_scan_start(void);

#endif
