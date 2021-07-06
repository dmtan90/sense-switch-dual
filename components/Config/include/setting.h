/*
  * setting
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#ifndef MAIN_h
#define MAIN_h

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//define configuration values
#define COMPANY_NAME                    "VKIST"
#define PRODUCT_NAME                    "SENSE SWICTH DUAL"
#define PRODUCT_RELEASE_DATE            1608734797635

#define HARDWARE_VERSION                "AH_SS_DUALR1"
#define SOFTWARE_VERSION                "0.0.1.4"
#define SOFTWARE_DATE                   1622944044842

#define AP_SSID                         "SENSE SWITCH"
#define AP_PWD                          "12345678"

#define MQTT_SERVER                     "iot.example.com"
#define MQTT_PORT                       1883
#define MQTT_USERNAME                   "example"
#define MQTT_PASSWORD                   "example.com"

enum SWUpdateMode{
	SW_UPDATE_MODE_DEVELOPING,
	SW_UPDATE_MODE_ALPHA,
	SW_UPDATE_MODE_BETA,
	SW_UPDATE_MODE_STABLE
};

enum SWBootMode{
	SW_BOOT_MODE_RUN,
	SW_BOOT_MODE_OTA_UPDATE,
	SW_BOOT_MODE_OTA_INIT
};

struct sw_update_struct{
  char update_version[20] = "";
  SWUpdateMode update_state = SW_UPDATE_MODE_STABLE;
  char update_description[512] = "";
  char update_url[256] = "";
  bool is_active = false;
  uint64_t update_date = 0;
};

typedef void (*CallBackTypeVoid)();
typedef void (*CallBackTypeString)(const char*, const size_t);
typedef void (*CallBackTypeChar)(const uint8_t*, const size_t);
typedef void (*CallBackTypeUpdate)(sw_update_struct*);

#endif
