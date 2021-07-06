/*
  * wifi_scan
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#ifndef ESP32WIFISCAN_H_
#define ESP32WIFISCAN_H_

extern "C" {
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_event.h>
//#include <esp32-hal.h>
#include <lwip/ip_addr.h>
#include "lwip/err.h"
}

class WiFiScanClass
{

public:

    int8_t scanNetworks(bool async = false, bool show_hidden = true);

    int8_t scanComplete();
    void scanDelete();

    // scan result
    bool getNetworkInfo(uint8_t networkItem, char* ssid, uint8_t &encryptionType, int32_t &RSSI, uint8_t* &BSSID, int32_t &channel);

    char* SSID(uint8_t networkItem);
    wifi_auth_mode_t encryptionType(uint8_t networkItem);
    int32_t RSSI(uint8_t networkItem);
    uint8_t * BSSID(uint8_t networkItem);
    char* BSSIDstr(uint8_t networkItem);
    int32_t channel(uint8_t networkItem);

    static void _scanDone();
protected:

    static bool _scanAsync;
    static bool _scanStarted;
    static bool _scanComplete;

    static uint16_t _scanCount;
    static void* _scanResult;

    static void * _getScanInfoByIndex(int i);

};


#endif /* ESP32WIFISCAN_H_ */
