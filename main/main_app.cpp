/*
  * main_app
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#include "wifi_manager.h"
#include "file_storage.h"
#include "mqtt_client_ex.h"
#include "sys_ctrl.h"
#include "esp_system.h"
#include "cse7761.h"
#include "smart_switch_connector.h"


extern "C"{
#include "captdns.h"
}
const char tag[]  = "Gateway PRO";

extern "C" void app_main(void) {
    ESP_LOGI(tag, "SOFTWARE_VERSION=%s", SOFTWARE_VERSION);
    ESP_LOGI(tag, "SOFTWARE_DATE=%s", uint642Char(SOFTWARE_DATE));
    
    gpio_pad_select_gpio(RELAY_1_PIN);
    gpio_pad_select_gpio(RELAY_2_PIN);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(RELAY_1_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(RELAY_2_PIN, GPIO_MODE_OUTPUT);

    /* Turn off all channels on startup */
    gpio_set_level(RELAY_1_PIN, 0);
    gpio_set_level(RELAY_2_PIN, 0);

    nvs_flash_init();
    file_storage::init();
    sys_ctrl::init();
    
    Config *cfg = Config::getInstance();
    cfg->setChannelState(CONTROLLER_CHANNEL_1, false);
    cfg->setChannelState(CONTROLLER_CHANNEL_2, false);

    wifi_manager::getInstance()->startService();
    Cse7761::getInstance();
    
    switch(sys_ctrl::getRunningMode())
    {
        case SYSTEM_MODE_CONFIG:
        {
            ESP_LOGI(tag, "Running mode: SYSTEM_MODE_CONFIG");
            web_server::getInstance()->start();
            captdnsInit();
            break;
        }

        case SYSTEM_MODE_OPERATION:
        {
            ESP_LOGI(tag, "Running mode: SYSTEM_MODE_OPERATION");
            mqtt_client_ex::getInstance()->start();
            break;
        }

        default:
        {
            ESP_LOGE(tag, "Code shoudn't touch here");
            break;
        }
    }
    ESP_LOGI(tag, "Free mem=%d", esp_get_free_heap_size());
};




