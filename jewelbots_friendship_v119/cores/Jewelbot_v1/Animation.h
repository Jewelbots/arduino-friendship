
#ifndef __ANIMATION_H__
#define __ANIMATION_H__


#include <stdbool.h>
#include <stdint.h>
#include "LED_Colors.h"

class Animation {
  public:
    Animation();
    ~Animation();
    void breathe_single_color(ColorLabel color);
    void jewelbots_logo(void);
    void rainbows(void);
    void bouncing_ball(void);
 };


#endif
