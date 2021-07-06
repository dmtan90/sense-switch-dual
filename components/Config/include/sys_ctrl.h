/*
  * sys_ctrl
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#ifndef __sys_ctrl_h_
#define __sys_ctrl_h_

#include "config.h"

extern "C" {
    #include <stdio.h>
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "esp_system.h"
    #include "nvs_flash.h"
    #include "driver/gpio.h"
}

class sys_ctrl
{
private:
    static const gpio_num_t BTN_CONFIG_PIN = SW_PIN;
    static const uint8_t TIME_PRESS_BTN_IN_SECOND = 3;


public:
    static void init();
    static RUNNING_MODE_CONFIG getRunningMode();
    static void setRunningMode(RUNNING_MODE_CONFIG mode);
    static bool resetFactory();
    static void rebootSystem(uint8_t seconds = 0);
    static void rebootTask(void* data);

private:
    static void handleInterruptGpio(void* arg);
    static void showMessage(void* mesg);
    static void gotoConfigMode(void* data);
    static void toggleChannel(void* data);

private:
    static const char s_Tag[];
    static uint8_t s_TimerCountInSecond;
};

#endif //__sys_ctrl_h_