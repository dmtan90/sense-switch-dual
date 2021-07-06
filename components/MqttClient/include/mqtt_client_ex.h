/*
  * mqtt_client
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#ifndef __MQTT_CLIENT_h__
#define __MQTT_CLIENT_h__

#include "config.h"

extern "C"
{
    #include "esp_system.h"
    #include "esp_event.h"
    #include "mqtt_client.h"
}

class mqtt_client_ex
{
public:
    static const uint8_t MAX_TOPIC_LEN                = 128;
    static const uint16_t SIZE_OF_BUFFER              = 1024*10;
    static const uint8_t UPDATE_DATA_INTERVAL_TIME    = 1; //Update every 1 minute
    static const uint8_t SYNC_PROFILE_INTERVAL_TIME   = 5; //Update every 5 minutes
public:
    void start();
    void pollData();
    void setData(char* data, uint16_t len, uint16_t offset, uint16_t maxlen);
    void sendMessage(const char* res);
    static mqtt_client_ex* getInstance();
private:
    mqtt_client_ex();
    ~mqtt_client_ex() {};
    static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
    static esp_err_t eventHandler(esp_mqtt_event_handle_t event);
    //static void startTask(void * pvParameters);
    static void startPollingTask(void * pvParameters);
    void handleMessage(const char* res, size_t len);

    static mqtt_client_ex* s_pInstance;
    const static char s_Tag[];

public:
    static TaskHandle_t xHandle;
    char buffer[SIZE_OF_BUFFER];
    char topic_subscribe[MAX_TOPIC_LEN];
    char topic_publish[MAX_TOPIC_LEN];
    char app_name[MAX_TOPIC_LEN];
    bool close;
    uint16_t mUpdateCounter;
    uint64_t mLastUpdate;
    esp_mqtt_client_handle_t mClient;
};

#endif