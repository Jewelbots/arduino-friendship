#ifndef _HAPTICS_DRIVER_H_
#define _HAPTICS_DRIVER_H_
void haptics_init(void);
void haptics_active(void);
void haptics_standby(void);
void haptics_enable(void);
void haptics_disable(void);
unsigned char haptics_test_cal_diags(void);
unsigned char haptics_test_run1(void);
unsigned char haptics_test_run2(void);
unsigned char haptics_test_run3(void);
unsigned char haptics_test_run4(void);
unsigned char haptics_msg_extra_short(void);
unsigned char haptics_msg_short(void);
unsigned char haptics_msg_medium(void);
unsigned char haptics_msg_long(void);
unsigned char haptics_msg_extra_long(void);
unsigned char haptics_msg_really_long(void);
unsigned char haptics_custom(uint8_t amplitude, uint32_t duration);

#endif
