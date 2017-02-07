/******************************************************************************
 * WaveformData.c
 *
 * Created on: April 23, 2012
 * Board: DRV2604EVM-CT, DRV2605EVM-CT
 *
 * Desc: This file contains the data for use with the Actuator_Waveforms.h
 *
 ******************************************************************************/

#include "WaveformData.h"
#include "DRV2605.h"

#define SCROLL 50

const unsigned char DRV2604_HEADER[] = {
    DRV2604_RAMDATA,  // Address for RAM entry, Begin loading data at DRV2604
                      // register zero
    0x00,             // RAM Library Revision Byte (to track library revisions)
    0x01, 0x00, 0x04, // Waveform #1 Strong Click = 45 ms
    0x01, 0x04, 0x04, // Waveform #2 Medium Click = 40 ms
    0x01, 0x08, 0x04, // Waveform #3 Light Click = 40 ms
    0x01, 0x0C, 0x04, // Waveform #4 Tick = 10 ms
    0x01, 0x10, 0x02, // Waveform #5 Bump = 40 ms
    0x01, 0x00, 0x24, // Waveform #6 Double Strong Click = 90 ms
    0x01, 0x04, 0x24, // Waveform #7 Double Medium Click = 80 ms
    0x01, 0x08, 0x24, // Waveform #8 Double Light Click = 80 ms
    0x01, 0x00, 0x44, // Waveform #9 Triple Strong Click = 135 ms
    0x01, 0x12, 0x04, // Waveform #A Buzz Alert = 750 ms
    0x01, 0x16, 0x06, // Waveform #B Ramp Up = 500 ms
    0x01, 0x1C, 0x06, // Waveform #C Ramp Down = 500 ms
    0x01, 0x22, 0x68, // Waveform #D Gallop Alert = 220 ms
    0x01, 0x2A, 0x6A, // Waveform #E Pulsing Alert = 340 ms
    0x01, 0x34, 0x04, // Waveform #F Test Click with Braking
    0x01, 0x38, 0x06, // Waveform 0x10 Test Buzz with Braking
    0x01, 0x38, 0xE6, // Waveform 0x11 Life Test Buzz with Braking
    0x01, 0x3E, 0xE2, // Waveform 0x12 Life Test Continuous Buzz
    0x01, 0x40, 0x04, // Waveform 0x13 Message short
    0x01, 0x44, 0x04  // Waveform 0x14 Message long
};

// Data
const unsigned char DRV2604_DATA[] = {
    DRV2604_RAMDATA, // Address for RAM entry
    0x3F, 0x09, 0x41,
    0x08, // Strong Click
    0x30, 0x08, 0x41,
    0x08, // Medium Click
    0x10, 0x08, 0x41,
    0x08, // Light Click
    0x3F, 0x02, 0x00,
    0x08, // Tick
    0x3F,
    0x08, // Bump
    0x3F, 0x96, 0x41,
    0x08, // Buzz Alert
    0x80, 0x64, 0x3F, 0x02, 0x41,
    0x08, // Ramp Up - 500 ms
    0xBF, 0x64, 0x00, 0x02, 0x41,
    0x08, // Ramp Down - 500 ms
    0x3F, 0x08, 0x41, 0x20, 0x24, 0x04, 0x10,
    0x10, // Gallop Alert
    0x3F, 0x08, 0x41, 0x08, 0x80, 0x32, 0x3F, 0x02, 0x41,
    0x08, // Pulsing Alert
    0x3F, 0x08, 0x00,
    0x08, // Test Click with braking
    0x3F, 0xFF, 0x3F, 0x91, 0x00,
    0xC8, // Test Buzz with braking
    0x3F,
    0xFF, // Test Continuous Buzz
    0x3F, 0x2A, 0x41,
    0x08, // Message Short
    0x3F, 0x8E, 0x41,
    0x08 // Message Long
};
