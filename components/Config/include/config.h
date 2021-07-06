/*
  * config
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#ifndef CONFIG_h
#define CONFIG_h
#include "setting.h"

extern "C" {
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <esp_partition.h>
#include <nvs.h>
#include <driver/gpio.h>
}

#ifndef DB_CONFIG
#define DB_CONFIG

#define SENSE_SWITCH_DUAL           "Sense Switch Dual"

#define COLOR_DISCONNECTED_WIFI     0x000020
#define COLOR_CONNECTED_WIFI        0x002000
#define COLOR_DEVICE_CONNECTING     0x070b0f
#define COLOR_GATEWAY_IN_CONFIG     0x200000
#define COLOR_GATEWAY_IN_UPDATE     0x8e24aa

#define TXD_PIN (GPIO_NUM_26)
#define RXD_PIN (GPIO_NUM_25)

#define LED_PIN (GPIO_NUM_18)

#define SW_PIN (GPIO_NUM_0)

#define SW_1_PIN (GPIO_NUM_32)
#define SW_2_PIN (GPIO_NUM_33)
#define SW_3_PIN (GPIO_NUM_34)
#define SW_4_PIN (GPIO_NUM_35)

#define RELAY_1_PIN (GPIO_NUM_27)
#define RELAY_2_PIN (GPIO_NUM_14)
#define RELAY_3_PIN (GPIO_NUM_17)
#define RELAY_4_PIN (GPIO_NUM_19)

static const uint32_t WARNING_FREE_MEM  = 70000;
static const uint32_t CRITICAL_FREE_MEM = 50000;

enum DB_OPEN_MODE{
    READ_ONLY   = 1,
    READ_WRITE  = 2
};

enum CONTROLLER_STATE {
    CONTROLLER_STATE_ON,
    CONTROLLER_STATE_OFF,
    CONTROLLER_STATE_UNKNOWN
};

enum CONTROLLER_CHANNEL {
    CONTROLLER_CHANNEL_1        = 0,
    CONTROLLER_CHANNEL_2        = 1,
    CONTROLLER_CHANNEL_3        = 2,
    CONTROLLER_CHANNEL_4        = 3,
    CONTROLLER_CHANNEL_COUNT
};

enum CONTROLLER_TYPE {
    DEVICE_CMD_UNKNOW                   = 0,
    DEVICE_CMD_R1                       = 23,
    DEVICE_CMD_R2                       = 24,
    DEVICE_CMD_R3                       = 25,
    DEVICE_CMD_R4                       = 26
};

enum DB_DEVICE_TYPE {
    DB_DEVICE_TYPE_UNKNOWN          = 0,
    DB_DEVICE_TYPE_SENSOR           = 1,
    DB_DEVICE_TYPE_CONTROLLER       = 2,
    DB_DEVICE_TYPE_CONTAINER        = 3,
    DB_DEVICE_TYPE_GATEWAY          = 4
};

enum DB_DEVICE_NAME {
    DB_DEVICE_NAME_UNKNOWN                  =   0,
    DB_DEVICE_NAME_SENSE_SWITCH_DUAL        =   40,
    DB_DEVICE_NAME_COUNT
};

enum SENSOR_TYPE{
    SENSOR_TYPE_UNKNOWN             = 0,
    SENSOR_TYPE_VOLTAGE             = 23,
    SENSOR_TYPE_AMPERAGE            = 24,
    SENSOR_TYPE_POWER_ENERGY        = 25
};

enum DEVICE_STATE{
    DEVICE_CONNECTED                = 0,
    DEVICE_DISCONNECTED             = 1,
    DEVICE_ERROR                    = 2
};

enum EntityFieldType{
    ENTITY_FIELD_CHAR       = 0,
    ENTITY_FIELD_BOOL       = 1,
    ENTITY_FIELD_INT8_T     = 2,
    ENTITY_FIELD_UINT8_T    = 3,
    ENTITY_FIELD_INT16_T    = 4,
    ENTITY_FIELD_UINT16_T   = 5,
    ENTITY_FIELD_INT32_T    = 6,
    ENTITY_FIELD_UINT32_T   = 7,
    ENTITY_FIELD_INT64_T    = 8,
    ENTITY_FIELD_UINT64_T   = 9,
    ENTITY_FIELD_DOUBLE     = 10
};

enum LED_MODE
{
    LED_MODE_STATIC,
    LED_MODE_BLINK,
    LED_MODE_NOTIFY,
    LED_MODE_COUNT
};

enum BUZZ_MODE
{
    BUZZ_MODE_OFF,
    BUZZ_MODE_BEEP,
    BUZZ_MODE_STATIC,
    BUZZ_MODE_COUNT
};

enum RUNNING_MODE_CONFIG
{
    SYSTEM_MODE_CONFIG = 0,
    SYSTEM_MODE_OPERATION,
    SYSTEM_MODE_UPDATE
};

struct EntityField{
    char name[32]="";
    void* value = NULL;
    EntityFieldType type = ENTITY_FIELD_CHAR;
};

#endif

enum CFDataType{
    CF_DATA_TYPE_CHAR   = 0,
    CF_DATA_TYPE_INT8   = 1,
    CF_DATA_TYPE_UINT8  = 2,
    CF_DATA_TYPE_INT16  = 3,
    CF_DATA_TYPE_UINT16 = 4,
    CF_DATA_TYPE_INT32  = 5,
    CF_DATA_TYPE_UINT32 = 6,
    CF_DATA_TYPE_INT64  = 7,
    CF_DATA_TYPE_UINT64 = 8,
    CF_DATA_TYPE_FLOAT  = 9
};

struct ConfigStruct{
    char company_name[32] = COMPANY_NAME;
    char product_name[32] = PRODUCT_NAME;
    char product_serial[32] = "";
    uint64_t product_release_date = PRODUCT_RELEASE_DATE;

    char hw_version[32] = HARDWARE_VERSION;
    char sw_version[32] = SOFTWARE_VERSION;
    uint64_t sw_date = SOFTWARE_DATE;

    char ap_ssid[33] = AP_SSID;
    char ap_pwd[33] = AP_PWD;
    char sta_ssid[33] = "";
    char sta_pwd[33] = "";

    uint8_t boot_mode = SYSTEM_MODE_CONFIG;

    uint8_t channel_1_state = 1;
    uint8_t channel_2_state = 1;

    SWUpdateMode update_state = SW_UPDATE_MODE_STABLE;

    //for MQTT config
    char mqtt_server[33] = MQTT_SERVER;
    uint16_t mqtt_port = MQTT_PORT;
    char mqtt_username[33] = MQTT_USERNAME;
    char mqtt_password[33] = MQTT_PASSWORD;
};


#ifndef REBOOT_STASK
#define REBOOT_STASK

#define delay_ms(ts) vTaskDelay(ts/portTICK_RATE_MS)

static uint64_t char2Uint64(const char *str)
{
  uint64_t result = 0; // Initialize result
  // Iterate through all characters of input string and update result
  for (int i = 0; str[i] != '\0'; ++i){
    result = result*10 + str[i] - '0';
  }
  return result;
};

static char uint642CharBuf[20] = "";
static char* uint642Char(const uint64_t num)
{
    uint64_t temp = num;
  char buf[20];
  buf[0] = '\0';
  //memset(buf, '\0', 20);

  if (temp == 0)
  {
    buf[0] = '0';
  }

  while (temp > 0)
  {
    uint64_t q = temp/10;
    uint8_t val = temp - q*10;
    char t[5];
    sprintf(t, "%d", val);
    strcat(buf, t);
    temp = q;
  }
  int len = strlen(buf);
  for(int i = 0; i < len/2;i++)
  {
    char c = buf[i];
    buf[i] = buf[len - i - 1];
    buf[len - i - 1] = c;
  }
  uint642CharBuf[0] = '\0';
  strcpy(uint642CharBuf, buf);
  return uint642CharBuf;
};


#endif

class Config{
  public:
    Config();

    ~Config();

    void* readCF(const char* name, CFDataType type, size_t len = 128);

    bool writeCF(const char* name, const void* value, CFDataType type);

    bool restore();

    char* toJSString();

    char* getAPSSID();

    void setAPSSID(const char* ssid);

    char* getAPPWD();

    void setAPPWD(const char* pwd);

    char* getSTASSID();

    void setSTASSID(const char* ssid);

    char* getSTAPWD();

    void setSTAPWD(const char* pwd);

    RUNNING_MODE_CONFIG getBootMode();

    void setBootMode(RUNNING_MODE_CONFIG mode);

    char* getCompanyName();

    char* getProductName();

    void setProductName(const char* name);

    char* getProductSerial();

    void setProductSerial(const char* serial);

    uint64_t getProductReleaseDate();

    char* getHardwareVersion();

    char* getSoftwareVersion();

    uint64_t getSWUpdatedDate();

    SWUpdateMode getUpdateState();

    void setUpdateState(SWUpdateMode state);

    void setChannelState(CONTROLLER_CHANNEL channel, bool state);

    bool getChannelState(CONTROLLER_CHANNEL channel);

    void commitChannelState();

    char* getMqttServer();

    void setMqttServer(const char* server);

    uint16_t getMqttPort();
    
    void setMqttPort(const uint16_t port);

    char* getMqttUser();

    void setMqttUser(const char* username);

    char* getMqttPwd();

    void setMqttPwd(const char* pwd);

    static Config* getInstance();

  private:
    static void createAndTakeSemaphore();
    static void releaseSemaphore();
    
    ConfigStruct cfg;
    char buffer[512];

    static SemaphoreHandle_t xSemaphore;
};

#endif
