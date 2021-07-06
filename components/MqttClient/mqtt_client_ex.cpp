/*
  * mqtt_client
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#include "mqtt_client_ex.h"
#include "wifi_sta.h"
#include "sys_ctrl.h"
#include "smart_switch_connector.h"
#include "cse7761.h"

mqtt_client_ex* mqtt_client_ex::s_pInstance = NULL;
TaskHandle_t mqtt_client_ex::xHandle = NULL;
const char mqtt_client_ex::s_Tag[] = "mqtt_client_ex";

static char s_mqtt_server_broker[256] = "";
static char s_username[32] = "";
static char s_password[32] = "";

mqtt_client_ex::mqtt_client_ex()
{
    ESP_LOGD(s_Tag, "mqtt_client_ex()");
    char* sMac = wifi_sta::getInstance()->getMacAddress();
    uint64_t ts = wifi_sta::getInstance()->getTimeStamp();
    if(strcmp(sMac, "") == 0)
    {
        ESP_LOGE(s_Tag, "Cannot get mac address");
    }
    else
    {
        Config* cfg = Config::getInstance();
        sprintf(s_mqtt_server_broker, "mqtt://%s:%d", cfg->getMqttServer(), cfg->getMqttPort());
        strcpy(s_username, cfg->getMqttUser());
        strcpy(s_password, cfg->getMqttPwd());

        memset(topic_subscribe, 0, MAX_TOPIC_LEN);
        memset(topic_publish, 0, MAX_TOPIC_LEN);
        sprintf(topic_subscribe, "farmery/to-gateway/%s", sMac);
        sprintf(topic_publish, "farmery/gateway");
        sprintf(app_name, "ID_%s_%s", sMac, uint642Char(ts));
        ESP_LOGI(s_Tag, "Subscribing: %s", topic_subscribe);
        ESP_LOGI(s_Tag, "Publishing: %s", topic_publish);

        mUpdateCounter = 0;
        mLastUpdate = 0;
        close = true;
    }
}

mqtt_client_ex* mqtt_client_ex::getInstance()
{
    if(NULL == s_pInstance)
    {
        s_pInstance = new mqtt_client_ex();
    }

    return s_pInstance;
}

void mqtt_client_ex::pollData()
{
    if(s_pInstance->close == true)
    {
        s_pInstance->mUpdateCounter = 0;
        return;
    }

    uint64_t ts = wifi_sta::getInstance()->getTimeStamp();
    s_pInstance->mLastUpdate = ts;

    if(s_pInstance->mUpdateCounter == 0){
        char* sMac = wifi_sta::getInstance()->getMacAddress();
        char data_to_send[128] = "";
        sprintf(data_to_send, "{\"gateway_mac_address\":\"%s\", \"timestamp\":%s,\"mqtt_cmd\":\"sync_relay_states\"}", 
            sMac, uint642Char(ts));

        ESP_LOGD(s_Tag, "publishing data %d-%s", strlen(data_to_send), data_to_send);
        esp_mqtt_client_publish(s_pInstance->mClient, s_pInstance->topic_publish, data_to_send, strlen(data_to_send), 0, 0);
    }

    char* data = smart_switch_connector::getInstance()->toJsonString();
    uint16_t len = strlen(data) + 10;
    char data_to_send[len];
    memset(data_to_send, '\0', len);
    strcat(data_to_send, data);

    //reset power energy after commiting data 
    Cse7761 *cse7761 = Cse7761::getInstance();
    cse7761->mEnergy.total = 0;

    ESP_LOGD(s_Tag, "publishing data %d-%s", strlen(data_to_send), data_to_send);
    esp_mqtt_client_publish(s_pInstance->mClient, s_pInstance->topic_publish, data_to_send, strlen(data_to_send), 0, 0);

    if(s_pInstance->mUpdateCounter >= 10800)
    {
        s_pInstance->mUpdateCounter = 0;
    }
    else
    {
        ++s_pInstance->mUpdateCounter;  
    }
};

esp_err_t mqtt_client_ex::eventHandler(esp_mqtt_event_handle_t event)
{
    int eventId = event->event_id;
    ESP_LOGD(s_Tag, "eventHandler id=%d", eventId);
    esp_mqtt_client_handle_t client = event->client;

    int msg_id = event->msg_id;

    switch (eventId) {
        case MQTT_EVENT_CONNECTED:
            s_pInstance->mUpdateCounter = 0;
            s_pInstance->close = false;
            msg_id = esp_mqtt_client_subscribe(client, s_pInstance->topic_subscribe, 0);
            ESP_LOGD(s_Tag, "sent subscribe successful, msg_id=%d", msg_id);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGE(s_Tag, "Connection closed");
            s_pInstance->close = true;
            s_pInstance->mUpdateCounter = 0;
            s_pInstance->start();
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGD(s_Tag, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", msg_id);
            //start polling data
            //s_pInstance->pollData();
            xTaskCreate(startPollingTask, "startPollingTask", 10*1024, NULL, tskIDLE_PRIORITY, NULL);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGD(s_Tag, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(s_Tag, "MQTT_EVENT_PUBLISHED, msg_id=%d", msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGD(s_Tag, "Got incoming message: %s", event->data);
            ESP_LOGD(s_Tag, "data_len: %d", event->data_len);
            ESP_LOGD(s_Tag, "Current_data_offset: %d", event->current_data_offset);
            ESP_LOGD(s_Tag, "Total_data_len: %d", event->total_data_len);
            s_pInstance->setData(event->data, event->data_len, event->current_data_offset, event->total_data_len);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGD(s_Tag, "MQTT_EVENT_ERROR");
            break;

        default:
            ESP_LOGD(s_Tag, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

void mqtt_client_ex::setData(char* data, uint16_t len, uint16_t offset, uint16_t maxlen)
{
    char temp[len+1] = "";
    strncpy(temp, data, len);
    if(offset == 0)
    {
        memset(buffer, '\0', SIZE_OF_BUFFER);
    }

    for(uint16_t i = 0; i < len; i++)
    {
        buffer[i+offset] = temp[i];
    }

    if(offset + len == maxlen)
    {
        handleMessage(&buffer[0], maxlen);
    }
};


void mqtt_client_ex::mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *data) {
    ESP_LOGD(s_Tag, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event_data = (esp_mqtt_event_handle_t)data;
    eventHandler(event_data);
}

void mqtt_client_ex::startPollingTask(void * pvParameters)
{
    do
    {
        ESP_LOGD(s_Tag, "startPollingTask - free mem=%d", esp_get_free_heap_size());
        if(s_pInstance->close == true)
        {
            break;
        }
        s_pInstance->pollData();
        delay_ms(60000);
    } while(1);

    vTaskDelete(NULL);
}

void mqtt_client_ex::start()
{
    while(false == wifi_sta::getInstance()->isConnected())
    {
        uint64_t ts = wifi_sta::getInstance()->getTimeStamp();
        s_pInstance->mLastUpdate = ts;
        delay_ms(60000);
    }

    ESP_LOGD(s_Tag, "start tick 1");

    esp_mqtt_client_config_t mqtt_cfg;
    memset(&mqtt_cfg, 0, sizeof(esp_mqtt_client_config_t));

    mqtt_cfg.uri = s_mqtt_server_broker;
    mqtt_cfg.username = s_username;
    mqtt_cfg.password = s_password;
    mqtt_cfg.client_id = mqtt_client_ex::app_name;

    ESP_LOGI(s_Tag, "Mqtt client connect to %s", s_mqtt_server_broker);
    s_pInstance->mClient = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(s_pInstance->mClient, MQTT_EVENT_ANY, mqtt_event_handler, s_pInstance->mClient);

    if (ESP_OK != esp_mqtt_client_start(s_pInstance->mClient))
    {
        ESP_LOGE(s_Tag, "mg_connect(%s) failed", s_mqtt_server_broker);
        delay_ms(60000);
        start();
    }
}

void mqtt_client_ex::sendMessage(const char* res)
{
    if(res != NULL && this->close == false)
    {
        char data_to_send[256] = "";
        sprintf(data_to_send, "data=%s", res);

        ESP_LOGD(s_Tag, "publishing data %d-%s", strlen(data_to_send), data_to_send);
        esp_mqtt_client_publish(this->mClient, this->topic_publish, data_to_send, strlen(data_to_send), 0, 0);
    }
}

void mqtt_client_ex::handleMessage(const char* res, size_t len)
{
    char data[len+1] = "";
    strncpy(data, res, len);
    if(strcmp(data, "[]") == 0)
    {
        ESP_LOGD(s_Tag, "No data");
        return;
    }
    
    if(strstr(data, "set_relay_state") != NULL)
    {
        smart_switch_connector::getInstance()->handleDeviceControlling(data);
    }
}
