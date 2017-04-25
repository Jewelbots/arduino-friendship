#ifndef ARDUINO_H_
#define ARDUINO_H_


#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

// Arduino specific code - C++
#include "JWB_Utils.h"
#include "LED.h"
#include "Buzzer.h"
#include "Animation.h"
#include "Timer.h"
#include "JWB_Serial.h"


#ifdef __cplusplus
extern "C"{
#endif // __cplusplus
#include "arm_math.h"
#include "utils.h"
#include "fsm.h"
#include "led_sequence.h"
#include "nrf_log.h"
#include "friend_data_storage.h"

/* sketch */
extern void setup( void ) ;
extern void loop( void ) ;

#ifdef __cplusplus
}
#endif

#endif
