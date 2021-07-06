/*
  * led_ctrl
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#ifndef __led_ctrl_h__
#define __led_ctrl_h__

#include "config.h"

extern "C" {
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/rmt_struct.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <stdio.h>
#include "ws2812.h"
}

class led_ctrl
{
public:
    struct s_led_ctrl_conf
    {
        LED_MODE        mode;
        rgb_config      color;
        uint32_t        delay_time;
    };
    static const uint32_t NOTIFY_TIMES = 8;
    static const uint32_t NOTIFY_TIME_DELAY_IN_MS = 100;
private:
    static const uint8_t DEFAULT_LED_PIN = LED_PIN;
    static const char s_Tag[];

    static const rgb_config init_color;
    static const rgb_config off_color;

public:
    static void init();
    static void setMode(uint32_t rgb, LED_MODE mode, uint32_t timing = 500);
    static void setNotify();
private:
    static void run();

    static void setColor();
private:
    static s_led_ctrl_conf s_running_conf;
};

#endif //__led_ctrl_h__