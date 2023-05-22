
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "mdns.h"

#define TAG     "SERVER"


static esp_err_t root_url_hit(httpd_req_t *req)
{
    ESP_LOGI(TAG,"URL:%s", req->uri);
    #if 1
    char* string = malloc(100);
    memset(string, '\0', 100);
    cJSON *resp = cJSON_CreateObject();
    if(NULL == resp)
    {
        return -1;
    }
    cJSON *packetType = cJSON_CreateNumber(1);
    if ( NULL == packetType)
    {
        return -1;
    }
    cJSON_AddItemToObject(resp, "packet", packetType);
    cJSON *connected = cJSON_CreateNumber(1);
    if (NULL == connected)
    {
        return -1;
    }
    cJSON_AddItemToObject(resp, "connected", connected);
    string = cJSON_Print(resp);
    cJSON_Delete(resp);
    httpd_resp_send(req, string, strlen(string));
    free(string);
    #endif
    //httpd_resp_sendstr(req, "Hello World!");
    return 0;
}

static esp_err_t on_post_wifi_config(httpd_req_t *req)
{
    return 0;
}

static esp_err_t on_capture_web_socket_url(httpd_req_t *req)
{
    return 0;
}

void register_end_points( void ){
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG, "Starting Server\n");
    if ( ESP_OK != httpd_start(&server, &config))
    {
        ESP_LOGE(TAG, "Could not Start Server\n");
    }

    httpd_uri_t rootEndPointConfig = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_url_hit,
    };
    httpd_register_uri_handler(server, &rootEndPointConfig);

    httpd_uri_t wifiEndPointConfig = {
        .uri = "/wifi_config",
        .method = HTTP_POST,
        .handler = on_post_wifi_config,
    };
    httpd_register_uri_handler(server, &wifiEndPointConfig);

    httpd_uri_t captureWebSocket = {
        .uri = "/ws_capture",
        .method = HTTP_GET,
        .handler = on_capture_web_socket_url,
        .is_websocket = true,
    };
    httpd_register_uri_handler(server, &captureWebSocket);
}

void start_mdns_service (void)
{
    mdns_init();
    mdns_hostname_set("targetsystem");
    mdns_instance_name_set("LEARN ESP32 MDNS");
}
