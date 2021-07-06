/*
  * wifi_manager
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#ifndef CONNECTION_h
#define CONNECTION_h

#include "config.h"
#include "setting.h"
#include "wifi_ap.h"
#include "wifi_sta.h"
#include "web_server.h"
#include "socket_ota_update.h"
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_event_legacy.h>
#include <esp_task_wdt.h>
#include <lwip/ip_addr.h>
#include <esp_wifi.h>
#include <freertos/task.h>
#include <freertos/list.h>

class wifi_manager
{
public:
    void init();
    void startService();
    void connectSTA();
    void createAP();

    static wifi_manager* getInstance();
    static char* mac_to_ip_address(const char* mac);
private:
    static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
private:
    wifi_manager();
    ~wifi_manager();

private:
    static wifi_manager* s_pInstance;
    static uint16_t s_staTimeout;
    static const char s_Tag[];
    static SemaphoreHandle_t s_Semaphore;
    static SemaphoreHandle_t s_SemaphoreMac2Ip;

};

#endif
