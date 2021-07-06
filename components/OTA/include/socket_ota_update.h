/*
  * socket_ota_update
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#ifndef __NETWORK_H__
#define __NETWORK_H__
extern "C"{
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
}

class Socketota_update{
public:
	Socketota_update();
	~Socketota_update();
	void init();
	void openSocket(const char *ssid, const char *password);
	int isConnected();
	static Socketota_update* getInstance();
};


#endif // __NETWORK_H__
