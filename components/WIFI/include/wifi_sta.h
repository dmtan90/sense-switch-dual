/*
  * wifi_sta
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#ifndef STATION_h
#define STATION_h

#include "setting.h"

extern "C"{
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include "mongoose.h"
#include <time.h>
#include <sys/time.h>
#include <lwip/apps/sntp.h>
}

class wifi_sta
{
public:
    void initialize_sntp();
    void init(char* ssid, char* pwd, CallBackTypeVoid success_cb, CallBackTypeVoid fail_cb);
    bool connectSTA(char* ssid, char* pwd, uint16_t timeout = 10);
    bool isConnected();
    void setConnected(bool isConnected=false);
    uint64_t getTimeStamp();
    uint8_t getMinutes();
    uint8_t getHours();
    //1 - 7:monday - sunday
    uint8_t getDayOfWeek();
    char* getMacAddress(bool isUpperCase = true);
    bool checkInternet();
private:
    wifi_sta();
    ~wifi_sta();

    static const char s_Tag[];
    char sMac[20];
private:
    bool connected, waiting, hasInternet;
    static wifi_sta* s_pInstance;

public:
    static wifi_sta* getInstance();
};

#endif
