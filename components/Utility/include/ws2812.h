/*
  * ws2812
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#ifndef WS2812_DRIVER_H
#define WS2812_DRIVER_H

#include <stdint.h>

typedef union {
    struct __attribute__ ((packed)) {
        uint8_t r, g, b;
    };
    uint32_t num;
} rgb_config;

extern void ws2812_init(int gpioNum);
extern void ws2812_setColors(uint16_t length, rgb_config *array);

inline rgb_config makeRGBVal(uint8_t r, uint8_t g, uint8_t b)
{
    rgb_config v;

    v.r = r;
    v.g = g;
    v.b = b;
    return v;
}

#endif /* WS2812_DRIVER_H */
