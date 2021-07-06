/*
  * wifi_ap
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#include "wifi_ap.h"
#include "wifi_sta.h"
#include "web_server.h"
#include "wifi_scan.h"
#include "config.h"

const char wifi_ap::s_Tag[] = "wifi_ap";
wifi_ap* wifi_ap::s_pInstance = NULL;

wifi_ap::wifi_ap()
{
    ESP_LOGD(s_Tag, "wifi_ap");
};

wifi_ap::~wifi_ap()
{
    ESP_LOGD(s_Tag, "~wifi_ap");
};

wifi_ap* wifi_ap::getInstance()
{
    if(NULL == s_pInstance)
    {
        s_pInstance = new wifi_ap();
    }
    return s_pInstance;
}

void wifi_ap::init(char* ap_ssid, char* ap_pwd)
{
    ESP_LOGD(s_Tag, "Starting being an access point SSID=%s PWD=%s PWDLEN=%d", ap_ssid, ap_pwd, strlen(ap_pwd));
    wifi_config_t wifi_config = {};

    wifi_config.ap.ssid_len = strlen(ap_ssid);
    memcpy(wifi_config.ap.ssid, ap_ssid, wifi_config.ap.ssid_len);
    memcpy(wifi_config.ap.password, ap_pwd, strlen(ap_pwd));
    if (strlen(ap_pwd) == 0) 
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    else
    {
        wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }

    wifi_config.ap.max_connection = 20;
    wifi_config.ap.channel = 1;
    wifi_config.ap.ssid_hidden = 0;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
};

bool wifi_ap::isCreated()
{
    return true;
};

