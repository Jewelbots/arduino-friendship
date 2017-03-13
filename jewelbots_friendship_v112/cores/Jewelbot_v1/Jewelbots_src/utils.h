#ifndef JEWELBOT_UTILS_H_
#define JEWELBOT_UTILS_H_
#include "ble_gap.h"
#include <stdbool.h>
#include <stdint.h>
void ble_address_to_string_convert(ble_gap_addr_t address,
                                   uint8_t *string_buffer);
bool flash_access_in_progress(void);
void set_arduino_coding(void);
void set_arduino_coding_false(void);
bool get_arduino_coding(void);

void set_arduino_button(void);
bool get_arduino_button(void);

#endif
