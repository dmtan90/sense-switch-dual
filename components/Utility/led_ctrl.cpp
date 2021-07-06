/*
  * led_ctrl
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#include "led_ctrl.h"

const rgb_config led_ctrl::init_color    = makeRGBVal(0x20, 0x0, 0x0);
const rgb_config led_ctrl::off_color    = makeRGBVal(0x0, 0x0, 0x0);

const char led_ctrl::s_Tag[]          = "LED";

led_ctrl::s_led_ctrl_conf led_ctrl::s_running_conf;

void led_ctrl::init()
{
    s_running_conf.mode          = LED_MODE_STATIC;
    s_running_conf.color         = init_color;
    s_running_conf.delay_time    = 0;

    ws2812_init(DEFAULT_LED_PIN);
    run();
}

void led_ctrl::setMode(uint32_t rgb, LED_MODE mode, uint32_t timing)
{
    s_running_conf.mode          = mode;
    s_running_conf.color.num     = rgb;
    s_running_conf.delay_time    = timing;
    run();
}

void led_ctrl::setNotify()
{
    s_running_conf.mode = LED_MODE_NOTIFY;
    run();
}

void led_ctrl::run()
{
    switch(s_running_conf.mode)
    {
        case LED_MODE_BLINK:
        {
            // set off led
            ws2812_setColors(1, (rgb_config*)&off_color);
            delay_ms(s_running_conf.delay_time);

            ws2812_setColors(1, &s_running_conf.color);
            delay_ms(s_running_conf.delay_time);

            // set off led
            ws2812_setColors(1, (rgb_config*)&off_color);
            delay_ms(s_running_conf.delay_time);

            ws2812_setColors(1, &s_running_conf.color);
            delay_ms(s_running_conf.delay_time);
            break;
        }

        case LED_MODE_STATIC:
        {
            ws2812_setColors(1, &s_running_conf.color);
            delay_ms(s_running_conf.delay_time);
            break;
        }

        case LED_MODE_NOTIFY:
        {
            for(uint32_t i = 0; i < NOTIFY_TIMES; ++i)
            {
                ws2812_setColors(1, (rgb_config*)&off_color);
                delay_ms(NOTIFY_TIME_DELAY_IN_MS);

                ws2812_setColors(1, &s_running_conf.color);
                delay_ms(NOTIFY_TIME_DELAY_IN_MS);
            }
            break;
        }

        default:
        {
            ESP_LOGE(s_Tag, "Code shoudn't touch here");
            break;
        }
    }
}