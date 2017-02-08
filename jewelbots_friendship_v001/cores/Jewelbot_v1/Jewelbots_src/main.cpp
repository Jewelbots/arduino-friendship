#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "app_button.h"
#include "app_error.h"
#include "app_scheduler.h"
#include "app_timer.h"
#include "app_timer_appsh.h"
#include "app_trace.h"
#include "app_uart.h"
#include "bootloader_types.h"
#include "ble.h"
#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "ble_conn_state.h"
#include "fds.h"
#include "fstorage.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_spi.h"
#include "nrf_drv_timer.h"
#include "nrf51_bitfields.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_twi.h"
#include "nrf_drv_wdt.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_soc.h"
#include "softdevice_handler_appsh.h"

#include "advertising.h"
#include "ble_central_event_handler.h"
#include "ble_lbs.h"
#include "button_handler.h"
#include "common_defines.h"
#include "db_discovery.h"
#include "friend_data_storage.h"
#include "fsm.h"
#include "Haptics.h"
#include "haptics_driver.h"
#include "hardfault.h"
#include "jewelbot_gpio.h"
#include "led_driver.h"
#include "jewelbot_service.h"
#include "jewelbot_types.h"
#include "jwb_dis.h"
#include "led_sequence.h"
#include "messaging.h"
#include "peer_management.h"
#include "pmic_driver.h"
#include "scan.h"
#include "jwb_twi.h"
#ifdef __cplusplus
}
#endif

// Arduino
#include "Arduino.h"



#define ARDUINO_MAIN

#if defined(NRF_LOG_USES_RTT) && NRF_LOG_USES_RTT == 1
#define RTT
#warning RTT Enabled: Do not Ship with RTT Enabled!
#endif
#if defined DEBUG
#warning DEBUG enabled: Do not ship with DEBUG Enabled!
#endif
#ifndef REAL_JEWELBOT
#warning REAL_JEWELBOT not enabled: Please enable REAL_JEWELBOT to build a release.
#endif

#define APP_TIMER_PRESCALER 0

#define SCHED_MAX_EVENT_DATA_SIZE                                              \
  ((CEIL_DIV(MAX(MAX(BLE_STACK_EVT_MSG_BUF_SIZE, ANT_STACK_EVT_STRUCT_SIZE),   \
                 SYS_EVT_MSG_BUF_SIZE),                                        \
             sizeof(uint32_t))) *                                              \
   sizeof(uint32_t))
#define SCHED_QUEUE_SIZE 45u

#ifdef REAL_JEWELBOT
static char *__attribute__((unused)) ident =
    "$Build: JEWELBOT " __DATE__ ", " __TIME__ " $";
#else
static char *__attribute__((unused)) ident =
    "$Build: DEVKIT " __DATE__ ", " __TIME__ " $";
#endif

static bool m_erase_bonds = false;

static void check_reset_reason() {
  uint32_t reset_reason = 0;
  sd_power_reset_reason_get(&reset_reason);
  NRF_LOG_PRINTF_DEBUG("Reset Reason: 0x%04x\r\n", reset_reason);
  if (reset_reason & POWER_RESETREAS_RESETPIN_Detected) {
    //logo_breathe();
    //delete_friends_list();
		NRF_LOG_DEBUG("In check reset reason\r\n");
		set_delete_friends_list();
    m_erase_bonds = true;
    sd_power_reset_reason_clr(POWER_RESETREAS_RESETPIN_Msk);
    nrf_delay_ms(1000);
  }
  else {
    m_erase_bonds = false;
  }
}



static void jewelbots_power_save(void) {
  clear_led();
  disable_led();
#ifdef REAL_JEWELBOT
  haptics_standby();
  haptics_disable();
#endif
}

static void scheduler_init(void) {
  APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}

static void timers_init(void) {
  APP_TIMER_APPSH_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, true);
}


int main(void) {
	bool  app_dfu = (NRF_POWER->GPREGRET == BOOTLOADER_DFU_END);

	if (app_dfu)
	{
			NRF_POWER->GPREGRET = 0;
	}
  ret_code_t err_code = 0;
  nrf_gpio_cfg_output(LED_RST);
  nrf_gpio_pin_clear(LED_RST);
  nrf_delay_us(10000);
  nrf_gpio_pin_set(LED_RST);
  nrf_delay_us(20000);
  err_code = NRF_LOG_INIT();
  APP_ERROR_CHECK(err_code);
  scheduler_init();
  timers_init();
  twi_init(); // 1
#ifdef REAL_JEWELBOT
  pmic_init(); // 2 //ifdef for nrf dev board
#endif
  services_init(); // 3
  boot_up_led_sequence();
  buttons_init();
#ifdef REAL_JEWELBOT
  haptics_init();
#endif
  ble_stack_init();
  db_discovery_init();
  gap_params_init();
  conn_params_init();
  jwb_dis_init();
  scan_init();
  set_ble_opts();
  init_advertising_module();
	check_reset_reason();
	peer_management_init(m_erase_bonds, app_dfu);
	jewelbot_state_machine_init();
  messaging_init();
  jewelbot_service_init();
  jewelbots_power_save();

  for (;;) {
    state_machine_dispatch();
    app_sched_execute();
#ifndef RTT
    ret_code_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
#endif
  }
}

/**
 * @}
 */
