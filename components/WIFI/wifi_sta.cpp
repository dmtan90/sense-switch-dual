/*
  * wifi_sta
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#include "wifi_sta.h"
#include "http_request.h"
#include "config.h"
#include "sys_ctrl.h"

const char wifi_sta::s_Tag[] = "wifi_sta";
wifi_sta* wifi_sta::s_pInstance = NULL;

wifi_sta::wifi_sta()
{
    ESP_LOGD(s_Tag, "wifi_sta");
    strcpy(sMac, "");
    getMacAddress();
};

wifi_sta::~wifi_sta()
{
    ESP_LOGD(s_Tag, "~wifi_sta");
};

wifi_sta* wifi_sta::getInstance()
{
    if (NULL == s_pInstance)
    {
        s_pInstance = new wifi_sta();
        s_pInstance->connected = false;
        s_pInstance->waiting = true;
    }
    return s_pInstance;
}

void wifi_sta::initialize_sntp(void)
{
    ESP_LOGD(s_Tag, "Initializing SNTP");
    //sntp_setoperatingmode(SNTP_OPMODE_POLL);
    char ntp[] = "time.google.com";
    sntp_setservername(0, ntp);
    sntp_init();

    // wait to get time from server
    char strftime_buf[64];
    time_t now = 0;
    int retry = 0;
    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(timeinfo));
    const int retry_count = 10;
    while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(s_Tag, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        delay_ms(2000);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if( retry < retry_count )
    {
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        ESP_LOGD(s_Tag, "Initializing SNTP Local time=%s", strftime_buf);
        setenv("TZ", "Etc/GMT-7", 1);
        tzset();
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        ESP_LOGD(s_Tag, "Initializing SNTP Local time=%s", strftime_buf);
    }
}

bool wifi_sta::checkInternet()
{
    ESP_LOGD(s_Tag, "checkInternet");
    hasInternet = false;
    waiting = true;
    if(connected == false)
    {
        return hasInternet;
    }

    http_request::get("http://vn.pool.ntp.org", "", [](const char* resp, size_t len){
        //ESP_LOGD(s_Tag, "resp=%s", resp);
        s_pInstance->hasInternet = true;
        s_pInstance->waiting = false;
    }, [](const char* resp, size_t len){
        s_pInstance->waiting = false;
    });

    while(waiting == true)
    {
        delay_ms(100);
    }

    return hasInternet;
}

void wifi_sta::init(char* ssid, char* pwd, CallBackTypeVoid success_cb, CallBackTypeVoid fail_cb)
{
    ESP_LOGD(s_Tag, "Connect to access point\n\rSSID: %s\n\rPWD: %s", ssid, pwd);
    connected = false;

    if(true == connectSTA(ssid, pwd, 15))
    {
        if(success_cb != NULL)
        {
            success_cb();
        }
    }
    else if(fail_cb != NULL)
    {
        fail_cb();
    }
};

bool wifi_sta::connectSTA(char* ssid, char* pwd, uint16_t timeout)
{
    bool ret = false;
    wifi_config_t sta_config;
    memset(&sta_config,0, sizeof(sta_config));
    strcpy((char*)sta_config.sta.ssid, ssid);
    strcpy((char*)sta_config.sta.password, pwd);
    
    if(connected)
    {
        ESP_ERROR_CHECK(esp_wifi_disconnect());
    }

    ESP_LOGI(s_Tag, "Connect to wifi: %s pass: %s", sta_config.sta.ssid, sta_config.sta.password);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));
    ESP_ERROR_CHECK(esp_wifi_connect());

    esp_err_t err = tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, "agrhub.com");
    if (err)
    {        
        ESP_LOGI(s_Tag, "tcpip_adapter_set_hostname failed, rc=%d", err);
    }

    do
    {
        --timeout;
        delay_ms(1000);
    } while(false == connected && 0 < timeout);
    
    return connected;
}

void wifi_sta::setConnected(bool isConnected)
{
    connected = isConnected;
}

bool wifi_sta::isConnected()
{
    return connected;
};

uint64_t wifi_sta::getTimeStamp()
{
    time_t seconds; 
    time(&seconds);
    unsigned long long millis = (unsigned long long)seconds * 1000;
    if(millis < SOFTWARE_DATE && connected)
    {
        initialize_sntp();
    }
    return millis;
}

char* wifi_sta::getMacAddress(bool isUpperCase)
{
    uint8_t mac[6] = {0};
    if(ESP_OK != esp_wifi_get_mac(WIFI_IF_STA, mac))
    {
        return sMac;
    }
    if(isUpperCase)
    {
        sprintf(sMac, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    else
    {
        sprintf(sMac, "%02x:%02x:%02x:%02x:%02x:%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    
    return sMac;   
}

uint8_t wifi_sta::getMinutes()
{
    time_t theTime = time(NULL);
    struct tm *aTime = localtime(&theTime);
    int min = aTime->tm_min;
    return min;
}

uint8_t wifi_sta::getHours()
{
    time_t theTime = time(NULL);
    struct tm *aTime = localtime(&theTime);
    int hour = aTime->tm_hour;
    return hour;
}

uint8_t wifi_sta::getDayOfWeek()
{
    time_t theTime = time(NULL);
    struct tm *aTime = localtime(&theTime);
    int wday = aTime->tm_wday;//0-6: days since Sunday
    if(wday == 0)
    {
        wday = 7;
    }
    return wday;
}




