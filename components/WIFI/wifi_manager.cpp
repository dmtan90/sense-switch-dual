/*
  * wifi_manager
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#include "wifi_manager.h"
#include "wifi_scan.h"
#include "led_ctrl.h"
#include "sys_ctrl.h"
//15 minutes
#define STA_CONNECT_TIMEOUT 300

const char wifi_manager::s_Tag[] = "wifi_manager";

wifi_manager* wifi_manager::s_pInstance = NULL;
SemaphoreHandle_t wifi_manager::s_Semaphore = NULL;
uint16_t wifi_manager::s_staTimeout = 0;

char* wifi_manager::mac_to_ip_address(const char* mac)
{
    ESP_LOGD(s_Tag, "mac_to_ip_address: mac=%s", mac);
    char* ip = NULL;
    wifi_sta_list_t stations;
    tcpip_adapter_sta_list_t infoList;
    tcpip_adapter_sta_info_t head;
    char buf[18];
    uint8_t retry = 0;
    do
    {
        ESP_ERROR_CHECK(esp_wifi_ap_get_sta_list(&stations));
        ESP_ERROR_CHECK(tcpip_adapter_get_sta_list(&stations, &infoList));
        for(int i = 0; i < infoList.num; i++)
        {
            head = infoList.sta[i];
            sprintf(buf, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x", 
                head.mac[0], head.mac[1], head.mac[2], head.mac[3], head.mac[4], head.mac[5]);
            //ESP_LOGD(s_Tag, "mac_to_ip_address item %d = %s", i, buf);
            if(strcmp(buf, mac) == 0)
            {
                ip = new char[32];
                sprintf(ip, "" IPSTR, IP2STR(&head.ip));
                break;
            }
        }

        if(ip != NULL && strcmp(ip, "0.0.0.0") == 0)
        {
            delete[] ip;
            ip = NULL;
            delay_ms(1000);
        }
        retry++;
    }while(ip == NULL && retry < 5);

    if(NULL != ip)
    {
        ESP_LOGD(s_Tag, "mac_to_ip_address ip=%s", ip);
    }
    return ip;
}

void wifi_manager::event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if(event_base == WIFI_EVENT)
    {
        switch(event_id)
        {
            case WIFI_EVENT_AP_START:
            {
                ESP_LOGD(s_Tag, "AP has been start");
                break;
            }
            case WIFI_EVENT_AP_STOP:
            {
                delay_ms(3000);
                s_pInstance->createAP();
                break;
            }
            case WIFI_EVENT_STA_CONNECTED:
            {
                led_ctrl::setMode(COLOR_CONNECTED_WIFI, LED_MODE_BLINK);
                break;
            }
            case WIFI_EVENT_STA_STOP:
            case WIFI_EVENT_STA_DISCONNECTED:
            {
                uint32_t freemem = esp_get_free_heap_size();
                if(sys_ctrl::getRunningMode() == SYSTEM_MODE_OPERATION && freemem < 10000)
                {
                    sys_ctrl::rebootSystem(0);
                }
                
                led_ctrl::setMode(COLOR_DISCONNECTED_WIFI, LED_MODE_STATIC);
                wifi_sta::getInstance()->setConnected(false);
                delay_ms(3000);
                ESP_ERROR_CHECK(esp_wifi_connect());
                if(sys_ctrl::getRunningMode() == SYSTEM_MODE_OPERATION)
                {
                    s_pInstance->s_staTimeout++;
                }

                if(s_pInstance->s_staTimeout > STA_CONNECT_TIMEOUT)
                {
                    sys_ctrl::rebootSystem(0);
                }
                break;
            }
            case WIFI_EVENT_SCAN_DONE:
            {
                WiFiScanClass::_scanDone();
                break;
            }
            case WIFI_EVENT_AP_STACONNECTED:
            {
                wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
                const uint8_t *sta_mac = event->mac;
                char* mac  = new char[64];
                sprintf(mac, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x", sta_mac[0], sta_mac[1], sta_mac[2], sta_mac[3], sta_mac[4], sta_mac[5]);
                ESP_LOGD(s_Tag, "WIFI_EVENT_AP_STACONNECTED JOINED mac=%s", mac);
                
                break;
            }
            case WIFI_EVENT_AP_STADISCONNECTED:
            {
                wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
                const uint8_t *sta_mac = event->mac;
                char* mac  = new char[64];
                sprintf(mac, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x", sta_mac[0], sta_mac[1], sta_mac[2], sta_mac[3], sta_mac[4], sta_mac[5]);
                ESP_LOGD(s_Tag, "SYSTEM_EVENT_AP_STADISCONNECTED LEFT mac=%s", mac);
                
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else if(event_base == IP_EVENT){
        switch(event_id)
        {
            case IP_EVENT_STA_GOT_IP:
            {
                ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
                //ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
                char ip[20];
                sprintf(ip, IPSTR, IP2STR(&event->ip_info.ip));

                ESP_LOGD(s_Tag, "SYSTEM_EVENT_STA_GOT_IP IP: %s", ip);
                wifi_sta::getInstance()->setConnected(true);
                led_ctrl::setMode(COLOR_CONNECTED_WIFI, LED_MODE_STATIC);
                wifi_sta::getInstance()->initialize_sntp();
                s_pInstance->s_staTimeout = 0;
                break;
            }
            default:
            {
                break;
            }
        }
    }
};

wifi_manager::wifi_manager()
{
    ESP_LOGD(s_Tag, "wifi_manager");
};

wifi_manager::~wifi_manager()
{
    ESP_LOGD(s_Tag, "~wifi_manager");
};

wifi_manager* wifi_manager::getInstance()
{
    if(s_pInstance == NULL){
        s_pInstance = new wifi_manager();
    }
    return s_pInstance;
};

void wifi_manager::startService()
{
    tcpip_adapter_init();
    //ESP_ERROR_CHECK(esp_netif_init());
    /*
     * for mesh
     * stop DHCP server on softAP interface by default
     * stop DHCP client on station interface by default
     */
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));

    ESP_ERROR_CHECK(tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA));
    ESP_ERROR_CHECK(tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA));

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    if(SYSTEM_MODE_CONFIG == sys_ctrl::getRunningMode())
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        createAP();
    }
    else
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    }
    
    if(SYSTEM_MODE_OPERATION == sys_ctrl::getRunningMode())
    {
        connectSTA();
    }
};

void wifi_manager::connectSTA()
{
    ESP_LOGD(s_Tag, "connectSTA");
    Config *cfg = Config::getInstance();
    char* ssid = cfg->getSTASSID();
    char* pwd = cfg->getSTAPWD();
    if(strlen(ssid) > 0)
    {
        wifi_sta::getInstance()->init(ssid, pwd, [](){
            ESP_LOGD(s_Tag, "connectSTA: successful");
        }, [](){
        });
    }
};

void wifi_manager::createAP()
{
    ESP_LOGD(s_Tag, "createAP");
    Config *cfg = Config::getInstance();
    uint8_t mac[6] = {0};
    if(ESP_OK != esp_wifi_get_mac(WIFI_IF_STA, mac))
    {
        ESP_LOGE(s_Tag, "createAP: can't get mac address");
    }
    char *ssid = new char[32];
    sprintf(ssid, "%s %02X%02X%02X", cfg->getAPSSID(), mac[3], mac[4], mac[5]);
    char* pwd = cfg->getAPPWD();
    wifi_ap::getInstance()->init(ssid, pwd);
    delete ssid;
};
