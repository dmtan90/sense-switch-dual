/*
 * fiot_smart_tank.h
 *
 *  Created on: 24 thg 5, 2017
 *      Author: dmtan
 */

#ifndef SMART_AC_CONTROLLER_H_
#define SMART_AC_CONTROLLER_H_

#include "config.h"

extern "C"{
#include <stdio.h>
}

struct ChannelState
{
    bool channel1;
    bool channel2;
    CONTROLLER_TYPE r1 = DEVICE_CMD_R1;
    CONTROLLER_TYPE r2 = DEVICE_CMD_R2;
};

class smart_switch_connector
{
public:
    ~smart_switch_connector();
    char* toJsonString();
    void getStates(ChannelState &states);
    void setControllerState(CONTROLLER_CHANNEL channel, bool state);
    bool getControllerState(CONTROLLER_CHANNEL channel);
    void toggleChannel(CONTROLLER_CHANNEL channel);
    void handleDeviceControlling(const char* json);
    ChannelState mState;
    char mJsonString[4096];
private:
    smart_switch_connector();
public:
    static smart_switch_connector* getInstance();
private:
    static smart_switch_connector* mInstance;
};

#endif /* SMART_SWITCH_CONTROLLER_H_ */
