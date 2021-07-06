/*
  * config
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#include "setting.h"
#include "config.h"
#include "wifi_sta.h"

const char tag[] = "CONFIG";

Config *config_instance = NULL;
SemaphoreHandle_t Config::xSemaphore = NULL;

Config* Config::getInstance(){
  if(config_instance == NULL){
    config_instance = new Config();
  }
  return config_instance;
};

Config::Config()
{
    uint8_t mac[6] = {0};
    if(ESP_OK != esp_wifi_get_mac(WIFI_IF_STA, mac))
    {
        ESP_LOGD(tag, "Cannot get mac address");
        mac[0] = 0;
        mac[1] = 0;
        mac[2] = 0;
        mac[3] = 0;
        mac[4] = 0;
        mac[5] = 0;
    }
    char *sMac = new char[32];
    sprintf(sMac, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    void* buff = (void*)sMac;//readCF("product_serial", CF_DATA_TYPE_CHAR);
    if(buff  != NULL)
    {
        setProductSerial((char*)buff);
        free(buff);
    }

    buff =  readCF("update_state", CF_DATA_TYPE_UINT8);
    if(buff != NULL)
    {
        uint8_t data = *((uint8_t *) buff);
        setUpdateState((SWUpdateMode)data);
    }

    buff =  readCF("sta_ssid", CF_DATA_TYPE_CHAR);
    if(buff != NULL)
    {
        setSTASSID((char*)buff);
        free(buff);
    }

    buff =  readCF("sta_pwd", CF_DATA_TYPE_CHAR);
    if(buff != NULL)
    {
        setSTAPWD((char*)buff);
        free(buff);
    }

    buff =  readCF("ap_ssid", CF_DATA_TYPE_CHAR);
    if(buff != NULL)
    {
        setAPSSID((char*)buff);
        free(buff);
    }

    buff = readCF("ap_pwd", CF_DATA_TYPE_CHAR);
    if(buff != NULL)
    {
        setAPPWD((char*)buff);
        free(buff);
    }

    buff =  readCF("boot_mode", CF_DATA_TYPE_UINT8);
    if(buff != NULL)
    {
        uint8_t data = *((uint8_t *) buff);
        setBootMode((RUNNING_MODE_CONFIG)data);
    }

    buff =  readCF("channel_1_state", CF_DATA_TYPE_UINT8);
    if(buff != NULL)
    {
        uint8_t data = *((uint8_t *) buff);
        setChannelState(CONTROLLER_CHANNEL_1, (bool)data);
    }

    buff =  readCF("channel_2_state", CF_DATA_TYPE_UINT8);
    if(buff != NULL)
    {
        uint8_t data = *((uint8_t *) buff);
        setChannelState(CONTROLLER_CHANNEL_2, (bool)data);
    }

    buff =  readCF("mqtt_server", CF_DATA_TYPE_CHAR);
    if(buff != NULL)
    {
        setMqttServer((char*)buff);
        free(buff);
    }

    buff =  readCF("mqtt_port", CF_DATA_TYPE_UINT16);
    if(buff != NULL)
    {
        uint16_t data = *((uint16_t *) buff);
        setMqttPort(data);
    }

    buff =  readCF("mqtt_username", CF_DATA_TYPE_CHAR);
    if(buff != NULL)
    {
        setMqttUser((char*)buff);
        free(buff);
    }

    buff =  readCF("mqtt_password", CF_DATA_TYPE_CHAR);
    if(buff != NULL)
    {
        setMqttPwd((char*)buff);
        free(buff);
    }
};

Config::~Config(){
    ESP_LOGD(tag, "~Config");
};

bool Config::restore(){
    printf("CF: restore");
    setSTASSID("");
    setSTAPWD("");
    setBootMode(SYSTEM_MODE_CONFIG);
    
    writeCF("sta_ssid", "", CF_DATA_TYPE_CHAR);
    writeCF("sta_pwd", "", CF_DATA_TYPE_CHAR);

    void *pointer = &cfg.boot_mode;
    writeCF("boot_mode", pointer, CF_DATA_TYPE_UINT8);

    pointer = &cfg.channel_1_state;
    writeCF("channel_1_state", pointer, CF_DATA_TYPE_UINT8);
    pointer = &cfg.channel_2_state;
    writeCF("channel_2_state", pointer, CF_DATA_TYPE_UINT8);

    setMqttServer(MQTT_SERVER);
    setMqttPort(MQTT_PORT);
    setMqttUser(MQTT_USERNAME);
    setMqttPwd(MQTT_PASSWORD);

    writeCF("mqtt_server", MQTT_SERVER, CF_DATA_TYPE_CHAR);
    writeCF("mqtt_username", MQTT_USERNAME, CF_DATA_TYPE_CHAR);
    writeCF("mqtt_password", MQTT_PASSWORD, CF_DATA_TYPE_CHAR);

    pointer = &cfg.mqtt_port;
    return writeCF("mqtt_port", pointer, CF_DATA_TYPE_UINT16);
};

char* Config::toJSString(){
    buffer[0] = '\0';
    char sw_date[20] = "";
    char hw_date[20] = "";
    strcpy(sw_date, uint642Char(getSWUpdatedDate()));
    strcpy(hw_date, uint642Char(getProductReleaseDate()));

    sprintf(buffer, "{\"hw_version\":\"%s\",\"sw_version\":\"%s\",\"sw_date\":%s, "\
               "\"ap_ssid\":\"%s\",\"ap_pwd\":\"%s\",\"sta_ssid\":\"%s\",\"sta_pwd\":\"%s\","\
               "\"company_name\":\"%s\",\"product_name\":\"%s\",\"product_serial\":\"%s\", "\
               "\"product_release_date\":%s,\"update_state\":%d,\"mqtt_server\":\"%s\", "\
               "\"mqtt_port\":%d,\"mqtt_username\":\"%s\",\"mqtt_password\":\"%s\"}",
          getHardwareVersion(), getSoftwareVersion(), sw_date,
          getAPSSID(), getAPPWD(), getSTASSID(), getSTAPWD(),
          getCompanyName(), getProductName(),  getProductSerial(),
          hw_date, getUpdateState(), getMqttServer(), getMqttPort(), getMqttUser(), getMqttPwd());

    ESP_LOGD(tag, "toJSString buf=%s", buffer);
    return buffer;
}

void* Config::readCF(const char* name, CFDataType type, size_t len)
{
    createAndTakeSemaphore();

    ESP_LOGD(tag, "readCF name=%s", name);

    void* buff = NULL;
    esp_err_t err = nvs_flash_init();

    if(err != ESP_OK)
    {
        releaseSemaphore();
        return buff;
    }

    nvs_handle my_handle;
    err = nvs_open("nvs", NVS_READONLY, &my_handle);
    if(err != ESP_OK)
    {
        releaseSemaphore();
        return buff;
    }

    switch(type)
    {
    case CF_DATA_TYPE_CHAR:
    {
        char* value = new char[len];
        memset(value, '\0', len);
        err = nvs_get_str(my_handle, name, value, &len);
        if(err == ESP_OK)
        {
            buff = (void*)value;
            ESP_LOGD(tag, "readCF value=%s", value);
        }
        break;
    }
    case CF_DATA_TYPE_INT8:
    {
        int8_t value = 0;
        err = nvs_get_i8(my_handle, name, &value);
        if(err == ESP_OK)
        {
            buff = &value;
        }
        break;
    }
    case CF_DATA_TYPE_UINT8:
    {
        uint8_t value = 0;
        err = nvs_get_u8(my_handle, name, &value);
        if(err == ESP_OK)
        {
            buff = &value;
        }
        break;
    }
    case CF_DATA_TYPE_INT16:
    {
        int16_t value = 0;
        err = nvs_get_i16(my_handle, name, &value);
        if(err == ESP_OK)
        {
            buff = &value;
        }
        break;
    }
    case CF_DATA_TYPE_UINT16:
    {
        uint16_t value = 0;
        err = nvs_get_u16(my_handle, name, &value);
        if(err == ESP_OK)
        {
            buff = &value;
        }
        break;
    }
    case CF_DATA_TYPE_INT32:
    {
        int32_t value = 0;
        err = nvs_get_i32(my_handle, name, &value);
        if(err == ESP_OK)
        {
            buff = &value;
        }
        break;
    }
    case CF_DATA_TYPE_UINT32:
    {
        uint32_t value = 0;
        err = nvs_get_u32(my_handle, name, &value);
        if(err == ESP_OK)
        {
            buff = &value;
        }
        break;
    }
    case CF_DATA_TYPE_INT64:
    {
        int64_t value = 0;
        err = nvs_get_i64(my_handle, name, &value);
        if(err == ESP_OK)
        {
            buff = &value;
        }
        break;
    }
    case CF_DATA_TYPE_UINT64:
    {
        uint64_t value = 0;
        err = nvs_get_u64(my_handle, name, &value);
        if(err == ESP_OK)
        {
            buff = &value;
        }
        break;
    }
    case CF_DATA_TYPE_FLOAT:
    {
        float value = -1;
        size_t ulen = 10;
        char* cValue = new char[ulen];
        memset(cValue, '\0', ulen);
        err = nvs_get_str(my_handle, name, cValue, &ulen);
        if(err == ESP_OK)
        {
            value = atof(cValue);
        }
        delete[] cValue;
        buff = &value;
        break;
    }
    default:
        break;
    }

    // Close
    nvs_close(my_handle);

    releaseSemaphore();

    return buff;
};

bool Config::writeCF(const char* name, const void* value, CFDataType type)
{
    createAndTakeSemaphore();

    ESP_LOGD(tag, "writeCF name=%s", name);

    bool result = false;
    esp_err_t err = nvs_flash_init();
    if(err != ESP_OK)
    {
        releaseSemaphore();
        return false;
    }

    nvs_handle my_handle;

    err = nvs_open("nvs", NVS_READWRITE, &my_handle);
    if(err != ESP_OK)
    {
        releaseSemaphore();
        return false;
    }

    switch(type)
    {
    case CF_DATA_TYPE_CHAR:
    {
        err = nvs_set_str(my_handle, name, (char*)value);
        break;
    }
    case CF_DATA_TYPE_INT8:
    {
        int8_t data = *((int8_t *) value);
        ESP_LOGD(tag, "writeCF value=%d", data);
        err = nvs_set_i8(my_handle, name, data);
        break;
    }
    case CF_DATA_TYPE_UINT8:
    {
        uint8_t data = *((uint8_t *) value);
        ESP_LOGD(tag, "writeCF value=%d", data);
        err = nvs_set_u8(my_handle, name, data);
        break;
    }
    case CF_DATA_TYPE_INT16:
    {
        int16_t data = *((int16_t *) value);
        ESP_LOGD(tag, "writeCF value=%d", data);
        err = nvs_set_i16(my_handle, name, data);
        break;
    }
    case CF_DATA_TYPE_UINT16:
    {
        uint16_t data = *((uint16_t *) value);
        ESP_LOGD(tag, "writeCF value=%d", data);
        err = nvs_set_u16(my_handle, name, data);
        break;
    }
    case CF_DATA_TYPE_INT32:
    {
        int32_t data = *((int32_t *) value);
        ESP_LOGD(tag, "writeCF value=%d", data);
        err = nvs_set_i32(my_handle, name, data);
        break;
    }
    case CF_DATA_TYPE_UINT32:
    {
        uint32_t data = *((uint32_t *) value);
        ESP_LOGD(tag, "writeCF value=%d", data);
        err = nvs_set_u32(my_handle, name, data);
        break;
    }
    case CF_DATA_TYPE_INT64:
    {
        int64_t data = *((int64_t *) value);
        //ESP_LOGD(tag, "writeCF value=%s", data);
        err = nvs_set_i64(my_handle, name, data);
        break;
    }
    case CF_DATA_TYPE_UINT64:
    {
        uint64_t data = *((uint64_t *) value);
        //ESP_LOGD(tag, "writeCF value=%d", data);
        err = nvs_set_u64(my_handle, name, data);
        break;
    }
    case CF_DATA_TYPE_FLOAT:
    {
        float data = *((float *) value);
        char buffer[10];
        int ret = snprintf(buffer, sizeof buffer, "%.2f", data);

        if (ret > 0 && ret <= sizeof buffer) 
        {
            err = nvs_set_str(my_handle, name, (char*)buffer);
        }

        break;
    }
    default:
        err = ESP_FAIL;
        break;
    }

    if(err == ESP_OK)
    {
        err = nvs_commit(my_handle);
        if(err != ESP_OK)
        {
            releaseSemaphore();
            return false;
        }
    }
    
    // Close
    nvs_close(my_handle);

    if(err == ESP_OK)
    {
        result = true;
    }

    releaseSemaphore();

    return result;
};

char* Config::getAPSSID(){
    ESP_LOGD(tag, "getAPSSID: %s", cfg.ap_ssid);
    return cfg.ap_ssid;
};

void Config::setAPSSID(const char* ssid){
    strcpy(cfg.ap_ssid, ssid);
    getAPSSID();
};

char* Config::getAPPWD(){
    ESP_LOGD(tag, "getAPPWD: %s", cfg.ap_pwd);
    return cfg.ap_pwd;
};

void Config::setAPPWD(const char* pwd){
    strcpy(cfg.ap_pwd, pwd);
    getAPPWD();
};

char* Config::getSTASSID(){
    ESP_LOGD(tag, "getSTASSID: %s", cfg.sta_ssid);
    return cfg.sta_ssid;
};

void Config::setSTASSID(const char* ssid){
    strcpy(cfg.sta_ssid, ssid);
    getSTASSID();
};

char* Config::getSTAPWD(){
    ESP_LOGD(tag, "getSTAPWD: %s", cfg.sta_pwd);
    return cfg.sta_pwd;
};

void Config::setSTAPWD(const char* pwd){
    strcpy(cfg.sta_pwd, pwd);
    getSTAPWD();
};

RUNNING_MODE_CONFIG Config::getBootMode()
{
    ESP_LOGD(tag, "getBootMode: %d", cfg.boot_mode);
    return (RUNNING_MODE_CONFIG)cfg.boot_mode;
}

void Config::setBootMode(RUNNING_MODE_CONFIG mode)
{
    cfg.boot_mode = mode;
    getBootMode();
}

char* Config::getCompanyName(){
    ESP_LOGD(tag, "getCompanyName: %s", cfg.company_name);
    return cfg.company_name;
};

char* Config::getProductName(){
    ESP_LOGD(tag, "getProductName: %s", cfg.product_name);
    return cfg.product_name;
};

char* Config::getProductSerial(){
    if(strcmp(cfg.product_serial, "00:00:00:00:00:00") == 0 || strlen(cfg.product_serial) == 0)
    {
        strcpy(cfg.product_serial, wifi_sta::getInstance()->getMacAddress());
    }
    ESP_LOGD(tag, "getProductSerial: %s", cfg.product_serial);
    return cfg.product_serial;
};

void Config::setProductSerial(const char* serial){
    strcpy(cfg.product_serial, serial);
    getProductSerial();
};

uint64_t Config::getProductReleaseDate(){
    ESP_LOGD(tag, "getProductReleaseDate: %llu", cfg.product_release_date);
    return cfg.product_release_date;
};

char* Config::getHardwareVersion()
{
    ESP_LOGD(tag, "getHardwareVersion: %s", cfg.hw_version);
    return cfg.hw_version;
};

char* Config::getSoftwareVersion()
{
    ESP_LOGD(tag, "getSoftwareVersion: %s", cfg.sw_version);
    return cfg.sw_version;
};

uint64_t Config::getSWUpdatedDate(){
    ESP_LOGD(tag, "getSWUpdatedDate: %llu", cfg.sw_date);
    return cfg.sw_date;
};

SWUpdateMode Config::getUpdateState(){
    ESP_LOGD(tag, "getUpdateState: %d", cfg.update_state);
    return cfg.update_state;
};

void Config::setUpdateState(SWUpdateMode state){
    cfg.update_state = state;
    getUpdateState();
};


char* Config::getMqttServer()
{
    return cfg.mqtt_server;
};

void Config::setMqttServer(const char* server)
{
    strcpy(cfg.mqtt_server, server);
};

uint16_t Config::getMqttPort()
{
    return cfg.mqtt_port;
};

void Config::setMqttPort(const uint16_t port)
{
    cfg.mqtt_port = port;
};

char* Config::getMqttUser()
{
    return cfg.mqtt_username;
};

void Config::setMqttUser(const char* username)
{
    strcpy(cfg.mqtt_username, username);
};

char* Config::getMqttPwd()
{
    return cfg.mqtt_password;
};

void Config::setMqttPwd(const char* pwd)
{
    strcpy(cfg.mqtt_password, pwd);
};

void Config::setChannelState(CONTROLLER_CHANNEL channel, bool state)
{
    if(channel == CONTROLLER_CHANNEL_1)
    {
        cfg.channel_1_state = state;
    }
    else if(channel == CONTROLLER_CHANNEL_2)
    {
        cfg.channel_2_state = state;
    }
    getChannelState(channel);
}
    
bool Config::getChannelState(CONTROLLER_CHANNEL channel)
{
    ESP_LOGD(tag, "getChannelState: channel=%d", channel);
    bool state = false;
    if(channel == CONTROLLER_CHANNEL_1)
    {
        state = cfg.channel_1_state;
    }
    else if(channel == CONTROLLER_CHANNEL_2)
    {
        state = cfg.channel_2_state;
    }
    return state;
}


void Config::commitChannelState()
{
    void *pointer = &cfg.channel_1_state;
    writeCF("channel_1_state", pointer, CF_DATA_TYPE_UINT8);
    delay_ms(1000);
    pointer = &cfg.channel_2_state;
    writeCF("channel_2_state", pointer, CF_DATA_TYPE_UINT8);
}

void Config::createAndTakeSemaphore()
{
    if(NULL == xSemaphore)
    {
        xSemaphore = xSemaphoreCreateMutex();
    }

    while( xSemaphoreTake( xSemaphore, ( TickType_t ) 100 ) == 0 );
}

void Config::releaseSemaphore()
{
    xSemaphoreGive( xSemaphore );
}

