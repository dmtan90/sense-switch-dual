/*
  * rgb_led
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#ifndef RGB_DRIVER_H
#define RGB_DRIVER_H

#include <stdint.h>
extern "C" {
#include "ws2812.h"
}

extern void rgb_init();
extern void rgb_setColors(rgb_config array);

#endif /* RGB_DRIVER_H */
