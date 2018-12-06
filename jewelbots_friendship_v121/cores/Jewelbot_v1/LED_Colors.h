#ifndef __LED_COLORS_H__
#define __LED_COLORS_H__

#include "LED_ColorLabel.h"

extern "C"{
 // __cplusplus

struct Color
{
  Color( uint8_t r, uint8_t g, uint8_t b )
  {
    this->r = r;
    this->g = g;
    this->b = b;
  }

  uint8_t r, g, b;
};

const Color COLORS[15] = {
  Color( 0x00, 0x00, 0x00 ), // Off
  Color( 0x3F, 0x00, 0x00 ), // Red
  Color( 0x00, 0x3F, 0x00 ), // Green
  Color( 0x00, 0x00, 0x3F ), // Blue
  Color( 0x3F, 0x3F, 0x00 ), // Yellow
  Color( 0x3F, 0x00, 0x3F ), // Magenta
  Color( 0x00, 0x3F, 0x3F ), // Cyan
  Color( 0x3F, 0x3F, 0x3F ), // White
  Color( 0xFF, 0x45, 0x00), // Orange
  Color( 0xFF, 0x8C, 0x00), // Gold
  Color( 0xEE, 0x82, 0xEE), // Purple
  Color( 0xE9, 0x96, 0x7A), // Periwinkle
  Color( 0xB2, 0x22, 0x22), // Rose
  Color( 0x20, 0xB2, 0x50), // Ocean
  Color( 0x64, 0x95, 0xFA) // Sky
};


}

#endif