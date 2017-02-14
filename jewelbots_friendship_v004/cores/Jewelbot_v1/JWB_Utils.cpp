#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "Arduino.h"
#include "JWB_Utils.h"
#ifdef __cplusplus
extern "C"{
#endif // __cplusplus
#include "app_scheduler.h"
#include "nrf_soc.h"
#include "app_error.h"
#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

void jewelbots_run(void) {
  state_machine_dispatch();
  app_sched_execute();
  ret_code_t err_code = sd_app_evt_wait();
  APP_ERROR_CHECK(err_code);
}
