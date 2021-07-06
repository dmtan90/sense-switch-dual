/*
  * web_server
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#include "web_server.h"
#include "wifi_scan.h"
#include "wifi_sta.h"
#include "config.h"
#include "mongoose.h"
#include "ota_update.h"
#include "ArduinoJson.h"
#include "sys_ctrl.h"
#include "led_ctrl.h"

extern const uint8_t material_icons_svg_start[] asm("_binary_material_icons_svg_start");
extern const uint8_t material_icons_svg_end[] asm("_binary_material_icons_svg_end");

extern const uint8_t material_icons_woff2_start[] asm("_binary_material_icons_woff2_start");
extern const uint8_t material_icons_woff2_end[] asm("_binary_material_icons_woff2_end");

extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[] asm("_binary_favicon_ico_end");

extern const uint8_t logo_png_start[]   asm("_binary_logo_png_start");
extern const uint8_t logo_png_end[]   asm("_binary_logo_png_end");

extern const uint8_t jquery_js_start[] asm("_binary_jquery_min_js_start");
extern const uint8_t jquery_js_end[] asm("_binary_jquery_min_js_end");

extern const uint8_t materialize_1_js_start[] asm("_binary_materialize_1_min_js_start");
extern const uint8_t materialize_1_js_end[] asm("_binary_materialize_1_min_js_end");

extern const uint8_t materialize_2_js_start[] asm("_binary_materialize_2_min_js_start");
extern const uint8_t materialize_2_js_end[] asm("_binary_materialize_2_min_js_end");

extern const uint8_t init_js_start[] asm("_binary_init_min_js_start");
extern const uint8_t init_js_end[] asm("_binary_init_min_js_end");

extern const uint8_t core_js_start[] asm("_binary_core_js_start");
extern const uint8_t core_js_end[] asm("_binary_core_js_end");

extern const uint8_t lang_js_start[] asm("_binary_lang_min_js_start");
extern const uint8_t lang_js_end[] asm("_binary_lang_min_js_end");

extern const uint8_t style_min_css_start[]   asm("_binary_style_min_css_start");
extern const uint8_t style_min_css_end[]   asm("_binary_style_min_css_end");

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

static const struct mg_str GET = MG_MK_STR("GET");
static const struct mg_str POST = MG_MK_STR("POST");

const char web_server::s_Tag[] = "web_server";

web_server *web_server::s_pInstance = NULL;

static int is_method_equal(struct mg_str s1, struct mg_str s2)
{
    return s1.len == s2.len && memcmp(s1.p, s2.p, s2.len) == 0;
}

void web_server::handleConnectWifi(struct mg_connection *nc, http_message *hm)
{
    char ssid[32], pwd[64];

    /* Get form variables */
    mg_get_http_var(&hm->body, "ssid", ssid, sizeof(ssid));
    mg_get_http_var(&hm->body, "pwd", pwd, sizeof(pwd));

    esp_wifi_disconnect();
    led_ctrl::setNotify();
    bool isConnected = wifi_sta::getInstance()->connectSTA(ssid, pwd, 10);

    /* Send headers */
    mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8 \r\n\r\n");
    char res[20];
    memset(res, '\0', sizeof(res));
    sprintf(res, "{\"success\": %s}", wifi_sta::getInstance()->isConnected() ? "true" : "false");
    mg_send(nc, res, strlen(res));

    if(isConnected)
    {
        Config *cfg = Config::getInstance();
        cfg->writeCF("sta_ssid", ssid, CF_DATA_TYPE_CHAR);
        cfg->writeCF("sta_pwd", pwd, CF_DATA_TYPE_CHAR);

        sys_ctrl::setRunningMode(SYSTEM_MODE_OPERATION);
    }
}

void web_server::handleSetupMqtt(struct mg_connection *nc, http_message *hm)
{
    char mqtt_server[32], mqtt_port[10], mqtt_username[32], mqtt_password[32] = "";

    /* Get form variables */
    mg_get_http_var(&hm->body, "mqtt_server", mqtt_server, sizeof(mqtt_server));
    mg_get_http_var(&hm->body, "mqtt_port", mqtt_port, sizeof(mqtt_port));
    mg_get_http_var(&hm->body, "mqtt_username", mqtt_username, sizeof(mqtt_username));
    mg_get_http_var(&hm->body, "mqtt_password", mqtt_password, sizeof(mqtt_password));

    Config *cfg = Config::getInstance();

    cfg->writeCF("mqtt_server", mqtt_server, CF_DATA_TYPE_CHAR);
    cfg->writeCF("mqtt_username", mqtt_username, CF_DATA_TYPE_CHAR);
    cfg->writeCF("mqtt_password", mqtt_password, CF_DATA_TYPE_CHAR);

    uint16_t port = atoi(mqtt_port);

    void *pointer = &port;
    bool rs = cfg->writeCF("mqtt_port", pointer, CF_DATA_TYPE_UINT16);

    /* Send headers */
    mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8 \r\n\r\n");
    char res[32];
    memset(res, '\0', sizeof(res));
    sprintf(res, "{\"success\": %s}", rs ? "true" : "false");
    mg_send(nc, res, strlen(res));
};

void web_server::handleOTAPage(struct mg_connection *nc, http_message *hm)
{
    char html[512] = "";
    memset(html, '\0', sizeof(html));
    strcpy(html, "<html><body>"\
            "<form method='POST' action=\"/ota_update\" enctype=\"multipart/form-data\">"\
            "<h4>Input admin password</h4>"\
            "<input type='password' name='admin_password'></input>"\
            "<h4>Input the product serial</h4>"\
            "<input type='file' name='file'></input>"\
            "<br/><br/><button type='submit'>SUBMIT</button></form></body></html>");
    mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8 \r\n\r\n");
    mg_send(nc, html, strlen(html));
};

void web_server::handleNotFound(struct mg_connection *nc, http_message *hm)
{
    ESP_LOGD(s_Tag, "handleNotFound: => redirect to home page");
    handleContents(nc, hm, "/index.html");
};

void web_server::handleRebootDevice(struct mg_connection *nc, http_message *hm)
{
    /* Send headers */
    mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8 \r\n\r\n");
    char res[20]= "{\"success\": true}";
    mg_send(nc, res, strlen(res));
    sys_ctrl::rebootSystem(10); // wait until fw reset
};

void web_server::handleContents(struct mg_connection *nc, http_message *hm, const char* uri)
{
    //prevent crash when many requests run at the same time
    ESP_LOGD(s_Tag, "handleContents - free mem=%d", esp_get_free_heap_size());
    if(strstr(uri, "index.html") != NULL)
    {
        mg_printf(nc, "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nContent-Type: text/html; charset=UTF-8\r\nKeep-Alive: timeout=15, max=100 \r\n\r\n");
        mg_send(nc, index_html_start, index_html_end - index_html_start);
    }
    else if(strstr(uri, "jquery.min.js") != NULL)
    {
        mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: text/javascript; charset=UTF-8\r\nContent-Encoding: gzip\r\n\r\n");
        mg_send(nc, jquery_js_start, jquery_js_end - jquery_js_start);
    }
    else if(strstr(uri, "materialize.1.min.js") != NULL)
    {
        mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: text/javascript; charset=UTF-8\r\nContent-Encoding: gzip\r\n\r\n");
        mg_send(nc, materialize_1_js_start, materialize_1_js_end - materialize_1_js_start);
    }
    else if(strstr(uri, "materialize.2.min.js") != NULL)
    {
        mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: text/javascript; charset=UTF-8\r\nContent-Encoding: gzip\r\n\r\n");
        mg_send(nc, materialize_2_js_start, materialize_2_js_end - materialize_2_js_start);
    }
    else if(strstr(uri, "init.min.js") != NULL)
    {
        mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: text/javascript; charset=UTF-8\r\nContent-Encoding: gzip\r\n\r\n");
        mg_send(nc, init_js_start, init_js_end - init_js_start);
    }
    else if(strstr(uri, "core.js") != NULL)
    {
        mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: text/javascript; charset=UTF-8\r\n\r\n");
        mg_send(nc, core_js_start, core_js_end - core_js_start);
    }
    else if(strstr(uri, "lang.min.js") != NULL)
    {
        mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: text/javascript; charset=UTF-8\r\nContent-Encoding: gzip\r\n\r\n");
        mg_send(nc, lang_js_start, lang_js_end - lang_js_start);
    }
    else if(strstr(uri, "style.min.css") != NULL)
    {
        mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: text/css; charset=UTF-8\r\nContent-Encoding: gzip\r\n\r\n");
        mg_send(nc, style_min_css_start, style_min_css_end - style_min_css_start);
    }
    else if(strstr(uri, "material-icons.svg") != NULL)
    {
        mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: image/svg+xml\r\n\r\n");
        mg_send(nc, material_icons_svg_start, material_icons_svg_end - material_icons_svg_start);
    }
    else if(strstr(uri, "material-icons.woff2") != NULL)
    {
        mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: application/font-woff2\r\n\r\n");
        //fix webfont issue when read from esp32
        mg_send(nc, material_icons_woff2_start, material_icons_woff2_end - material_icons_woff2_start);

    }
    else if(strstr(uri, "logo.png") != NULL)
    {
        mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: image/png\r\n\r\n");
        mg_send(nc, logo_png_start, logo_png_end - logo_png_start);
    }
    else if(strstr(uri, "favicon.ico") != NULL)
    {
        mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: image/x-icon\r\n\r\n");
        mg_send(nc, favicon_ico_start, favicon_ico_end - favicon_ico_start);
    }
    else
    {
        ESP_LOGE(s_Tag, "page not found %s", uri);
        handleNotFound(nc, hm);
    }

};

void web_server::handleScanWiFi(struct mg_connection *nc)
{
    ESP_LOGD(s_Tag, "handleScanWiFi - start");

    // WiFi.scanNetworks will return the number of networks found
    WiFiScanClass WiFi;
    int n = WiFi.scanNetworks();
    ESP_LOGD(s_Tag, "handleScanWiFi - found=%d", n);
    uint16_t max_length = (n+1)*128;
    char json[max_length];
    json[0] = '\0';
    strcpy(json,"[");

    for (int i = 0; i < n; i++)
    {
        // Print SSID and RSSI for each network found
        if(i > 0){
            strcat(json,",");
        }
        char buf[128];
        char* ssid = WiFi.SSID(i);
        sprintf(buf, "{\"name\":\"%s\",\"signal\":\"%d\",\"encrypted\":%s}",
                ssid, WiFi.RSSI(i), WiFi.encryptionType(i) ? "true" : "false");
        delete[] ssid;
        strcat(json, buf);
    }
    strcat(json,"]");

    mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
    mg_send(nc, json, strlen(json));
};

void web_server::handleGetData(struct mg_connection *nc)
{
    ESP_LOGD(s_Tag, "handleGetData - start");
    Config *cfg = Config::getInstance();
    char* json = cfg->toJSString();
    mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
    mg_send(nc, json, strlen(json));
};

void web_server::handleFactoryReset(struct mg_connection *nc)
{
    ESP_LOGD(s_Tag, "handleFactoryReset");
    bool rs = sys_ctrl::resetFactory();
    char res[64];
    sprintf(res, "{\"success\": %s}", rs ? "true" : "false");
    mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
    mg_send(nc, res, strlen(res));
    if(rs)
    {
        sys_ctrl::rebootSystem(10);
    }
};

// Mongoose event handler.
void web_server::event_handler(struct mg_connection *nc, int ev, void *evData)
{
    uint64_t ts = wifi_sta::getInstance()->getTimeStamp();
    s_pInstance->mLastUpdate = ts;
    if(MG_EV_POLL != ev) ESP_LOGD(s_Tag, "ev=%d", ev);
    switch (ev)
    {
    case MG_EV_HTTP_REQUEST:
    {
        struct http_message *hm = (struct http_message *) evData;
        char *uri = new char[128];
        strncpy(uri, hm->uri.p, hm->uri.len);
        uri[hm->uri.len] = '\0';
        if(strcmp(uri, "/") == 0)
        {
            strcpy(uri, "/index.html");
        }
        else if(strcmp(uri, "/favicon.ico") == 0)
        {
            strcpy(uri, "/img/favicon.ico");
        }

        ESP_LOGD(s_Tag, "mongoose_event_handler uri=%s", uri);
        if(strstr(uri, ".html") != NULL
                || strstr(uri, ".js") != NULL
                || strstr(uri, ".css") != NULL
                || strstr(uri, ".woff2") != NULL
                || strstr(uri, ".ttf") != NULL
                || strstr(uri, ".png") != NULL
                || strstr(uri, ".ico") != NULL)
        {
            // mg_serve_http(nc, hm, opts);
            handleContents(nc, hm, uri);
        }
        else if(strstr(uri, "/update") != NULL)
        {
            if(is_method_equal(hm->method, GET))
            {
                handleOTAPage(nc, hm);
            }
        }
        else if(strstr(uri, "/api/v1/scan_wifi") != NULL)
        {
            handleScanWiFi(nc);
        }
        else if(strstr(uri, "/api/v1/connect_wifi") != NULL)
        {
            if(is_method_equal(hm->method, POST))
            {
                handleConnectWifi(nc, hm);
            }
            else
            {
                mg_printf(nc, "%s",
                            "HTTP/1.0 501 Not Implemented\r\n"
                            "Content-Length: 0\r\n\r\n");
            }
        }
        else if(strstr(uri, "/api/v1/setup_mqtt") != NULL)
        {
            if(is_method_equal(hm->method, POST))
            {
                handleSetupMqtt(nc, hm);
            }
            else
            {
                mg_printf(nc, "%s",
                            "HTTP/1.0 501 Not Implemented\r\n"
                            "Content-Length: 0\r\n\r\n");
            }
        }
        else if(strstr(uri, "/api/v1/get_data") != NULL)
        {
            handleGetData(nc);
        }
        else if(strstr(uri, "/api/v1/reboot_device") != NULL)
        {
            if(is_method_equal(hm->method, POST))
            {
                handleRebootDevice(nc, hm);
            }
            else
            {
                mg_printf(nc, "%s",
                            "HTTP/1.0 501 Not Implemented\r\n"
                            "Content-Length: 0\r\n\r\n");
            }
        }
        else if(strstr(uri, "/api/v1/factory_reset") != NULL)
        {
            if(is_method_equal(hm->method, POST))
            {
                handleFactoryReset(nc);
            }
            else
            {
                mg_printf(nc, "%s",
                            "HTTP/1.0 501 Not Implemented\r\n"
                            "Content-Length: 0\r\n\r\n");
            }
        }
        else
        {
            handleNotFound(nc, hm);
        }
        delete[] uri;
        nc->flags |= MG_F_SEND_AND_CLOSE;
        mbuf_remove(&nc->recv_mbuf, nc->recv_mbuf.len);
        break;
    }

    default:
        break;
    }

} // End of mongoose_event_handler

void web_server::handle_ota_update(struct mg_connection *nc, int ev, void *ev_data)
{
    ESP_LOGD(s_Tag, "handle_ota_update EV=%d", ev);
    struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) ev_data;

    ota_update *ota = ota_update::getInstance();

    switch (ev) {
    case MG_EV_HTTP_MULTIPART_REQUEST:
    {
        /* Get form variables */
        char adminPWD[32] = "";
        memset(adminPWD, '\0', sizeof(adminPWD));
        struct http_message *hm = (struct http_message *) ev_data;

        mg_get_http_var(&hm->query_string, "admin_password", adminPWD, sizeof(adminPWD));
        if(strcmp(adminPWD, "support@agrhub.com") != 0)
        {
            ESP_LOGD(s_Tag, "Check point 3");
            mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8 \r\n\r\n");
            char res[] = "{\"success\": false}";
            mg_send(nc, res, strlen(res));
            nc->flags |= MG_F_SEND_AND_CLOSE;
            return;
        }
        led_ctrl::setNotify();
        ota->begin();

        break;
    }

    case MG_EV_HTTP_PART_DATA:
    {
        int len = mp->data.len;
        ota->writeHexData(mp->data.p, len);
        break;
    }
    case MG_EV_HTTP_MULTIPART_REQUEST_END:
    {
        ota_update_result rs = ota->end();

        if(rs == OTA_OK)
        {
            led_ctrl::setNotify();
            ESP_LOGI(s_Tag, "OTA update successful");
        }
        else
        {
            ESP_LOGD(s_Tag, "handle_ota_update fail=%d", rs);
        }

        mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8 \r\n\r\n");
        char res[20];
        memset(res, '\0', sizeof(res));
        sprintf(res, "{\"success\": %s}", rs == 0 ? "true" : "false");
        mg_send(nc, res, strlen(res));
        nc->flags |= MG_F_SEND_AND_CLOSE;
        mbuf_remove(&nc->recv_mbuf, nc->recv_mbuf.len);
        if(rs == OTA_OK)
        {
            sys_ctrl::rebootSystem(); // wait until fw reset
        }
        break;
    }
    default:
        break;
    }
}

// FreeRTOS task to start Mongoose.
void web_server::startTask(void *data)
{
    ESP_LOGD(s_Tag, "Mongoose task starting");
    struct mg_mgr mgr;
    mg_mgr_init(&mgr, NULL);

    ESP_LOGD(s_Tag, "Mongoose: Succesfully inited");
    mg_connection *cn = mg_bind(&mgr, ":80", event_handler);
    if (cn == NULL)
    {
        vTaskDelete(NULL);
        sys_ctrl::rebootSystem();
        return;
    }

    ESP_LOGD(s_Tag, "Mongoose Successfully bound");

    //handle OTA local update
    mg_register_http_endpoint(cn, "/ota_update", web_server::handle_ota_update);
    mg_set_protocol_http_websocket(cn);

    for(;;)
    {
        ESP_LOGD(s_Tag, "startTask tick");
        ESP_LOGD(s_Tag, "startTask - free mem=%d", esp_get_free_heap_size());
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);

    sys_ctrl::rebootSystem(0);
    vTaskDelete(NULL);
}

web_server::web_server()
{
    ESP_LOGD(s_Tag, "web_server");
    uint64_t ts = wifi_sta::getInstance()->getTimeStamp();
    mLastUpdate = ts;
}

web_server::~web_server()
{
    ESP_LOGD(s_Tag, "~web_server");
    s_pInstance = NULL;
}

web_server* web_server::getInstance()
{
    if(s_pInstance == NULL)
    {
        s_pInstance = new web_server();
    }
    return s_pInstance;
}

void web_server::start()
{
    xTaskCreatePinnedToCore(startTask, "web_server_task", 9126, NULL, tskIDLE_PRIORITY + 1, NULL, 1);
}
