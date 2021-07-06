/*
  * http_request
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#include "mongoose.h"
#include "setting.h"

extern "C" {
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
}

enum HTTP_REQUEST_TYPE
{
    HTTP_REQUEST_TYPE_GET,
    HTTP_REQUEST_TYPE_POST,
    HTTP_REQUEST_TYPE_DELETE,
    HTTP_REQUEST_TYPE_GET_STREAM
};

class http_request
{
public:
    // ~http_request();
    //static http_request *getInstance();
    static void get(const char* url, const char* headers,
            CallBackTypeString successCallback,
            CallBackTypeString failCallback, uint8_t pTimeout=30);
    static  void getStream(const char* url, const char* headers,
            CallBackTypeString successCallback,
            CallBackTypeString failCallback, uint8_t pTimeout=30);
    static void post(const char* url, const char* params, const char* headers,
            CallBackTypeString successCallback,
            CallBackTypeString failCallback, uint8_t pTimeout=30);

    static void event_handler(struct mg_connection *nc, int ev, void *ev_data);

private:
    static void createAndTakeSemaphore();
    static void releaseSemaphore();

    static CallBackTypeString success_cb;
    static CallBackTypeString fail_cb;
    static bool isBusy;
    static uint32_t timeout;
    static HTTP_REQUEST_TYPE requestType;
    static size_t content_length;
    static int last_ev;
    static bool exit_flag;
    static SemaphoreHandle_t xSemaphore;
};
