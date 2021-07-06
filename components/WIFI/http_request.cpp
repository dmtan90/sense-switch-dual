/*
  * http_request
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#include "http_request.h"

const char s_Tag[] = "HTTP_REQUEST";
uint8_t TIMEOUT = 15;

CallBackTypeString http_request::success_cb = NULL;
CallBackTypeString http_request::fail_cb = NULL;
uint32_t http_request::timeout = TIMEOUT;
bool http_request::isBusy = false;
HTTP_REQUEST_TYPE http_request::requestType = HTTP_REQUEST_TYPE_GET;
size_t http_request::content_length = 0;
bool http_request::exit_flag = false;
SemaphoreHandle_t http_request::xSemaphore = NULL;
int http_request::last_ev = MG_EV_POLL;

void http_request::event_handler(struct mg_connection *nc, int ev, void *ev_data)
{
    struct http_message *hm = (struct http_message *) ev_data;
    int connect_status;
    ESP_LOGD(s_Tag, "event_handler ev=%d", ev);
    switch (ev)
    {
        case MG_EV_CONNECT:
        {
            connect_status = *(int *) ev_data;
            if(connect_status != ESP_OK)
            {
                char error[32];
                sprintf(error, "Connection error = %d", connect_status);
                http_request::fail_cb(error, 0);
                nc->flags |= MG_F_SEND_AND_CLOSE;
            }
            content_length = 0;
            break;
        }
        case MG_EV_HTTP_REPLY:
        {
            if(http_request::requestType != HTTP_REQUEST_TYPE_GET_STREAM)
            {
                nc->flags |= MG_F_SEND_AND_CLOSE;
                if(hm->resp_code == 200)
                {
                    http_request::success_cb(hm->body.p, hm->body.len);
                }
                else
                {
                    http_request::fail_cb(hm->body.p, hm->body.len);
                }
            }
            break;
        }
        case MG_EV_RECV:
        {
            if(http_request::requestType == HTTP_REQUEST_TYPE_GET_STREAM)
            {
                size_t len = nc->recv_mbuf.len;

                char* buf = nc->recv_mbuf.buf;
                char* ptr = strstr(nc->recv_mbuf.buf, "Content-Length: ");
                if(ptr != NULL)
                {
                    //parse header
                    char templen[10];
                    char* ptr2 = strstr(ptr, "\r\n");
                    ptr = ptr+sizeof("Content-Length:");
                    strncpy(templen, ptr, ptr2-ptr);

                    buf = strstr(buf, "\r\n\r\n") + 4;
                    content_length = atoi(templen);
                    len = len - (buf - nc->recv_mbuf.buf);
                    http_request::success_cb("BEGIN", 0);
                    ptr = NULL;
                }
                content_length = content_length - len;
                http_request::success_cb(buf, len);
                mbuf_remove(&nc->recv_mbuf, nc->recv_mbuf.size);
                ESP_LOGD(s_Tag, "content_length=%d", content_length);
                if(content_length <= 0)
                {
                    http_request::success_cb("END", 0);
                    http_request::exit_flag = 1;
                    nc->flags |= MG_F_SEND_AND_CLOSE;
                    http_request::success_cb = NULL;
                }
                ptr = NULL;
                buf = NULL;
            }
            else
            {
                char* buf = nc->recv_mbuf.buf;
                if(strstr(buf, "HTTP/1.1 200") == NULL)
                {
                    nc->flags |= MG_F_CLOSE_IMMEDIATELY;
                    char templen[32];
                    char* ptr = strstr(buf, "HTTP/1.1");
                    char* ptr2 = strstr(ptr, "\r\n");
                    strncpy(templen, ptr, ptr2-ptr);
                    http_request::fail_cb(templen, 0);
                    http_request::exit_flag = 1;
                }
            }

            break;
        }
        case MG_EV_CLOSE:
        {
            ESP_LOGD(s_Tag, "Server closed connection");
            nc->flags |= MG_F_CLOSE_IMMEDIATELY;
            http_request::exit_flag = 1;
            mbuf_remove(&nc->recv_mbuf, nc->recv_mbuf.len);
            break;
        }

        case MG_EV_POLL:
        {
            if(last_ev == ev)
            {
                http_request::timeout--;
                if(http_request::timeout == 0)
                {
                    http_request::fail_cb("Timeout", 0);
                    nc->flags |= MG_F_CLOSE_IMMEDIATELY;
                }
            }
            else
            {
                http_request::timeout = TIMEOUT;
            }
            break;
        }

        default:
            break;
    }

    last_ev = ev;
};

void http_request::get(const char* url, const char* headers,
        CallBackTypeString successCallback, CallBackTypeString failCallback, uint8_t pTimeout)
{
    createAndTakeSemaphore();
    requestType = HTTP_REQUEST_TYPE_GET;
    success_cb = successCallback;
    fail_cb = failCallback;
    exit_flag = 0;
    timeout = pTimeout;
    TIMEOUT = pTimeout;

    ESP_LOGD(s_Tag, "get - Starting RESTful client against %s", url);

    struct mg_mgr mgr;

    mg_mgr_init(&mgr, NULL);
    mg_connection *cn = mg_connect_http(&mgr, http_request::event_handler, url, headers, NULL);
    if(cn == NULL)
    {
        return fail_cb("Can't create connection", 0);
    }

    while (exit_flag == 0)
    {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);
    releaseSemaphore();
};

void http_request::getStream(const char* url, const char* headers,
        CallBackTypeString successCallback, CallBackTypeString failCallback, uint8_t pTimeout)
{
    createAndTakeSemaphore();

    requestType = HTTP_REQUEST_TYPE_GET_STREAM;
    success_cb = successCallback;
    fail_cb = failCallback;
    exit_flag = 0;
    timeout = pTimeout;
    TIMEOUT = pTimeout;

    struct mg_mgr mgr;

    mg_mgr_init(&mgr, NULL);
    mg_connection *cn = mg_connect_http(&mgr, http_request::event_handler, url, headers, NULL);
    if(cn == NULL)
    {
        return fail_cb("Can't create connection", 0);
    }

    ESP_LOGD(s_Tag, "getStream - Starting RESTful client against %s", url);
    while (exit_flag == 0)
    {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);
    ESP_LOGD(s_Tag, "getStream - done");
    releaseSemaphore();
};

void http_request::post(const char* url, const char* params, const char* headers,
        CallBackTypeString successCallback, CallBackTypeString failCallback, uint8_t pTimeout) {

    createAndTakeSemaphore();
    requestType = HTTP_REQUEST_TYPE_POST;
    success_cb = successCallback;
    fail_cb = failCallback;
    struct mg_mgr mgr;
    exit_flag = 0;
    timeout = pTimeout;
    TIMEOUT = pTimeout;
    char extra_header[256] = "";
    sprintf(extra_header,
            "Content-Type: application/x-www-form-urlencoded\r\n%s", headers);

    mg_mgr_init(&mgr, NULL);

    mg_connection *cn = mg_connect_http(&mgr, http_request::event_handler,
            url, extra_header, params);
    if (cn == NULL) {
        return fail_cb("Can't create connection", 0);
    }

    ESP_LOGD(s_Tag, "post - Starting RESTful client against %s", url);
    while (exit_flag == 0) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);

    releaseSemaphore();
}

void http_request::createAndTakeSemaphore()
{
    if(NULL == xSemaphore)
    {
        xSemaphore = xSemaphoreCreateMutex();
    }

    while( xSemaphoreTake( xSemaphore, ( TickType_t ) 100 ) == 0 );
}

void http_request::releaseSemaphore()
{
    xSemaphoreGive( xSemaphore );
}





