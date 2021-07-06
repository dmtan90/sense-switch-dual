/*
  * web_server
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#ifndef COMPONENTS_WIFI_INCLUDE_WEB_SERVER_H_
#define COMPONENTS_WIFI_INCLUDE_WEB_SERVER_H_
#include "mongoose.h"
#include "sdkconfig.h"
extern "C" {
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <nvs_flash.h>
#include <nvs.h>
}

class web_server
{
private:
    web_server();
    virtual ~web_server();

public:
    uint64_t mLastUpdate;
    void start();
    static web_server* getInstance();

private:
    static void handleGetData(struct mg_connection *nc);
    static void handleOTAPage(struct mg_connection *nc, http_message *hm);
    static void handleCheckNewFirmware(struct mg_connection *nc);
    static void handleRebootDevice(struct mg_connection *nc, http_message *hm);
    static void handleConnectWifi(struct mg_connection *nc, http_message *hm);
    static void handleSetupMqtt(struct mg_connection *nc, http_message *hm);
    static void handleContents(struct mg_connection *nc, http_message *hm, const char* uri);
    static void handleNotFound(struct mg_connection *nc, http_message *hm);
    static void handleScanWiFi(struct mg_connection *nc);
    static void handleFactoryReset(struct mg_connection *nc);
    static void startTask(void *data);
    static void event_handler(struct mg_connection *nc, int ev, void *evData);

    static void handleRunota_updateFirmware(struct mg_connection *nc, http_message *hm);
    static void handleGetSystemStatistic(struct mg_connection *nc);
    static void handle_ota_update(struct mg_connection *nc, int ev, void *ev_data);

private:
    static web_server *s_pInstance;
    static const char s_Tag[];
};

#endif /* COMPONENTS_WIFI_INCLUDE_WEB_SERVER_H_ */
