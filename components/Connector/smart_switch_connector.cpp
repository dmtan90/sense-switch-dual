/*
 * fiot_smart_tank.cpp
 *
 *  Created on: 24 thg 5, 2017
 *      Author: dmtan
 */

#include "smart_switch_connector.h"
#include "wifi_manager.h"
#include "ArduinoJson.h"
#include "cse7761.h"

const char TAG[] = "SMART_SWITCH_CONTROLLER";
smart_switch_connector* smart_switch_connector::mInstance = NULL;

smart_switch_connector::smart_switch_connector()
{
    ESP_LOGD(TAG, "smart_switch_connector");
    Config *cfg = Config::getInstance();

    mState.channel1 = cfg->getChannelState(CONTROLLER_CHANNEL_1);
    mState.channel2 = cfg->getChannelState(CONTROLLER_CHANNEL_2);
}

smart_switch_connector* smart_switch_connector::getInstance()
{
    if(NULL == mInstance)
    {
        mInstance = new smart_switch_connector();
    }

    return mInstance;
}


smart_switch_connector::~smart_switch_connector()
{
    ESP_LOGI(TAG, "~smart_switch_connector");
    mInstance = NULL;
}

void smart_switch_connector::getStates(ChannelState &states)
{
    ESP_LOGD(TAG, "getStates");
    states.channel1 = mState.channel1;
    states.channel2 = mState.channel2;
}

//run by user action
void smart_switch_connector::setControllerState(CONTROLLER_CHANNEL channel, bool state)
{
    ESP_LOGD(TAG, "setControllerState channel=%d state=%d", channel, state);

    switch(channel)
    {
        case CONTROLLER_CHANNEL_1:
        {
            gpio_set_level(RELAY_1_PIN, state ? 1 : 0);
            mState.channel1 = state;
            break;   
        }
        case CONTROLLER_CHANNEL_2:
        {
            gpio_set_level(RELAY_2_PIN, state ? 1 : 0);
            mState.channel2 = state;
            break;   
        }
        default:
        {
            ESP_LOGI(TAG, "TODO: implement later: %d", channel);
            break;
        }
    }
}

bool smart_switch_connector::getControllerState(CONTROLLER_CHANNEL channel)
{
    bool rs = false;

    switch(channel)
    {
        case CONTROLLER_CHANNEL_1:
        {
            rs = mState.channel1;
            break;   
        }
        case CONTROLLER_CHANNEL_2:
        {
            rs = mState.channel2;
            break;   
        }
        default:
        {
            ESP_LOGI(TAG, "TODO: implement later: %d", channel);
            break;
        }
    }
    return rs;
}

char* smart_switch_connector::toJsonString()
{
    char state_1[6] = "false";
    char state_2[6] = "false";
    if (true == mState.channel1)
    {
        strcpy(state_1, "true");
    }

    if (true == mState.channel2)
    {
        strcpy(state_2, "true");
    }

    Cse7761 *cse7761 = Cse7761::getInstance();
    uint16_t U = cse7761->mEnergy.voltage[0];
    float I = cse7761->mEnergy.current[0] + cse7761->mEnergy.current[1];
    float P = cse7761->mEnergy.total;

    char jsonDeviceString[1024] = "";

    sprintf(jsonDeviceString, "["
            "{\"sensor_type\":%d,\"sensor_value\":%d},"
            "{\"sensor_type\":%d,\"sensor_value\":%.02f},"
            "{\"sensor_type\":%d,\"sensor_value\":%.0f},"
            "{\"controller_channel\":%d, \"controller_type\":%d,\"controller_is_on\":%s},"
            "{\"controller_channel\":%d, \"controller_type\":%d,\"controller_is_on\":%s}"
        "]",
        SENSOR_TYPE_VOLTAGE, U,
        SENSOR_TYPE_AMPERAGE, I,
        SENSOR_TYPE_POWER_ENERGY, P,
        CONTROLLER_CHANNEL_1, mState.r1, state_1,
        CONTROLLER_CHANNEL_2, mState.r2, state_2);
    ESP_LOGD(TAG, "Data: %s", jsonDeviceString);

    char* sMac = wifi_sta::getInstance()->getMacAddress();
    uint64_t ts = wifi_sta::getInstance()->getTimeStamp();

    sprintf(mJsonString, "{"
            "\"gateway_mac_address\":\"%s\","
            "\"gateway_name\":%d,"
            "\"gateway_type\":%d,"
            "\"timestamp\":%s,"
            "\"freemem\":%d,"
            "\"devices\":[{\"device_name\":%d,\"device_type\":%d,\"device_state\":%d,\"device_mac_address\":\"%s\",\"data\":%s}]}",
        sMac,
        DB_DEVICE_NAME_SENSE_SWITCH_DUAL,
        DB_DEVICE_TYPE_CONTROLLER,
        uint642Char(ts),
        esp_get_free_heap_size(),
        DB_DEVICE_NAME_SENSE_SWITCH_DUAL,
        DB_DEVICE_TYPE_CONTROLLER,
        DEVICE_CONNECTED,
        sMac,
        jsonDeviceString);

    return mJsonString;
}

void smart_switch_connector::toggleChannel(CONTROLLER_CHANNEL channel)
{
    ESP_LOGD(TAG, "toggleChannel: %d", channel);
    bool state = this->getControllerState(channel);
    state = !state;
    this->setControllerState(channel, state);
}

void smart_switch_connector::handleDeviceControlling(const char* json)
{
    ESP_LOGD(TAG, "handleDeviceControlling");
    //fix issue leak memmory
    DynamicJsonBuffer jsonBuffer(4096);
    JsonObject& device = jsonBuffer.parseObject(json);
    const char* mac = device["device_mac_address"];
    const bool isUserTrigger = device["device_user_trigger"];
    JsonArray& arrData = device["data"];
    for(uint8_t index = 0; index < arrData.size(); ++index)
    {
        bool state = arrData[index]["controller_is_on"];
        uint8_t type = arrData[index]["controller_type"];
        CONTROLLER_CHANNEL channel = CONTROLLER_CHANNEL_1;
        if(type == DEVICE_CMD_R2)
        {
            channel = CONTROLLER_CHANNEL_2;
        }
        else if(type == DEVICE_CMD_R3)
        {
            channel = CONTROLLER_CHANNEL_3;
        }
        else if(type == DEVICE_CMD_R4)
        {
            channel = CONTROLLER_CHANNEL_4;
        }
        this->setControllerState(channel, state);
    }
}
