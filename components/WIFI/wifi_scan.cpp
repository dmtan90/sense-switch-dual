/*
  * wifi_scan
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#include "wifi_scan.h"
#include "wifi_type.h"
#include "config.h"

bool WiFiScanClass::_scanAsync = false;
bool WiFiScanClass::_scanStarted = false;
bool WiFiScanClass::_scanComplete = false;

uint16_t WiFiScanClass::_scanCount = 0;
void* WiFiScanClass::_scanResult = 0;

esp_err_t wifi_scan_event_handler(void *ctx, system_event_t *event)
{
    if (event->event_id == SYSTEM_EVENT_SCAN_DONE)
    {
        WiFiScanClass::_scanDone();
    }
    return ESP_OK;
} // wifi_event_handler

/**
 * Start scan WiFi networks available
 * @param async         run in async mode
 * @param show_hidden   show hidden networks
 * @return Number of discovered networks
 */
int8_t WiFiScanClass::scanNetworks(bool async, bool show_hidden)
{
    if(WiFiScanClass::_scanStarted) {
        return WIFI_SCAN_RUNNING;
    }

    WiFiScanClass::_scanAsync = async;

    //ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    scanDelete();

    //ESP_ERROR_CHECK(esp_event_loop_init(wifi_scan_event_handler, NULL));

    wifi_scan_config_t config;
    memset(&config, 0, sizeof(config));
    config.show_hidden  = show_hidden;
    config.scan_type    = WIFI_SCAN_TYPE_ACTIVE;

    if(esp_wifi_scan_start(&config, WiFiScanClass::_scanAsync) == ESP_OK)
    {
        WiFiScanClass::_scanComplete = false;
        WiFiScanClass::_scanStarted = true;

        if(WiFiScanClass::_scanAsync)
        {
            return WIFI_SCAN_RUNNING;
        }

        while(!WiFiScanClass::_scanComplete)
        {
            delay_ms(10);
        }
        return WiFiScanClass::_scanCount;
    }
    else
    {
        return WIFI_SCAN_FAILED;
    }

}


/**
 * private
 * scan callback
 * @param result  void *arg
 * @param status STATUS
 */
void WiFiScanClass::_scanDone()
{
    WiFiScanClass::_scanComplete = true;
    WiFiScanClass::_scanStarted = false;
    esp_wifi_scan_get_ap_num(&(WiFiScanClass::_scanCount));
    if(WiFiScanClass::_scanCount) {
        WiFiScanClass::_scanResult = new wifi_ap_record_t[WiFiScanClass::_scanCount];
        if(WiFiScanClass::_scanResult) {
            esp_wifi_scan_get_ap_records(&(WiFiScanClass::_scanCount), (wifi_ap_record_t*)_scanResult);
        } else {
            //no memory
            WiFiScanClass::_scanCount = 0;
        }
    }
}

/**
 *
 * @param i specify from which network item want to get the information
 * @return bss_info *
 */
void * WiFiScanClass::_getScanInfoByIndex(int i)
{
    if(!WiFiScanClass::_scanResult || (size_t) i > WiFiScanClass::_scanCount) {
        return 0;
    }
    return reinterpret_cast<wifi_ap_record_t*>(WiFiScanClass::_scanResult) + i;
}

/**
 * called to get the scan state in Async mode
 * @return scan result or status
 *          -1 if scan not fin
 *          -2 if scan not triggered
 */
int8_t WiFiScanClass::scanComplete()
{

    if(_scanStarted) {
        return WIFI_SCAN_RUNNING;
    }

    if(_scanComplete) {
        return WiFiScanClass::_scanCount;
    }

    return WIFI_SCAN_FAILED;
}

/**
 * delete last scan result from RAM
 */
void WiFiScanClass::scanDelete()
{
    if(WiFiScanClass::_scanResult) {
        delete[] reinterpret_cast<wifi_ap_record_t*>(WiFiScanClass::_scanResult);
        WiFiScanClass::_scanResult = 0;
        WiFiScanClass::_scanCount = 0;
    }
    _scanComplete = false;
}


/**
 * loads all infos from a scanned wifi in to the ptr parameters
 * @param networkItem uint8_t
 * @param ssid  const char**
 * @param encryptionType uint8_t *
 * @param RSSI int32_t *
 * @param BSSID uint8_t **
 * @param channel int32_t *
 * @return (true if ok)
 */
bool WiFiScanClass::getNetworkInfo(uint8_t i, char* ssid, uint8_t &encType, int32_t &rssi, uint8_t* &bssid, int32_t &channel)
{
    wifi_ap_record_t* it = reinterpret_cast<wifi_ap_record_t*>(_getScanInfoByIndex(i));
    if(!it) {
        return false;
    }
    strcpy(ssid, (const char*) it->ssid);
    encType = it->authmode;
    rssi = it->rssi;
    bssid = it->bssid;
    channel = it->primary;
    return true;
}


/**
 * Return the SSID discovered during the network scan.
 * @param i     specify from which network item want to get the information
 * @return       ssid string of the specified item on the networks scanned list
 */
char* WiFiScanClass::SSID(uint8_t i)
{

    wifi_ap_record_t* it = reinterpret_cast<wifi_ap_record_t*>(_getScanInfoByIndex(i));
    if(it == NULL) {
        return NULL;
    }
    char* ssid = new char[33];
    strcpy(ssid, reinterpret_cast<const char*>(it->ssid));

    return ssid;
}


/**
 * Return the encryption type of the networks discovered during the scanNetworks
 * @param i specify from which network item want to get the information
 * @return  encryption type (enum wl_enc_type) of the specified item on the networks scanned list
 */
wifi_auth_mode_t WiFiScanClass::encryptionType(uint8_t i)
{
    wifi_ap_record_t* it = reinterpret_cast<wifi_ap_record_t*>(_getScanInfoByIndex(i));
    if(!it) {
        return WIFI_AUTH_OPEN;
    }
    return it->authmode;
}

/**
 * Return the RSSI of the networks discovered during the scanNetworks
 * @param i specify from which network item want to get the information
 * @return  signed value of RSSI of the specified item on the networks scanned list
 */
int32_t WiFiScanClass::RSSI(uint8_t i)
{
    wifi_ap_record_t* it = reinterpret_cast<wifi_ap_record_t*>(_getScanInfoByIndex(i));
    if(!it) {
        return 0;
    }
    return it->rssi;
}


/**
 * return MAC / BSSID of scanned wifi
 * @param i specify from which network item want to get the information
 * @return uint8_t * MAC / BSSID of scanned wifi
 */
uint8_t * WiFiScanClass::BSSID(uint8_t i)
{
    wifi_ap_record_t* it = reinterpret_cast<wifi_ap_record_t*>(_getScanInfoByIndex(i));
    if(!it) {
        return 0;
    }
    return it->bssid;
}

/**
 * return MAC / BSSID of scanned wifi
 * @param i specify from which network item want to get the information
 * @return String MAC / BSSID of scanned wifi
 */
char* WiFiScanClass::BSSIDstr(uint8_t i)
{

    wifi_ap_record_t* it = reinterpret_cast<wifi_ap_record_t*>(_getScanInfoByIndex(i));
    if(!it) {
        return NULL;
    }

    char *mac = new char[18];
    sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", it->bssid[0], it->bssid[1], it->bssid[2], it->bssid[3], it->bssid[4], it->bssid[5]);

    return mac;
}

int32_t WiFiScanClass::channel(uint8_t i)
{
    wifi_ap_record_t* it = reinterpret_cast<wifi_ap_record_t*>(_getScanInfoByIndex(i));
    if(!it) {
        return 0;
    }
    return it->primary;
}

