
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "mdns.h"

#define TAG     "SERVER"


static int client_session_id;
static httpd_handle_t server = NULL;

esp_err_t send_camera_data_ws(uint8_t* buf, size_t len){
if(!client_session_id)
{
    ESP_LOGE(TAG, "no client session id");
    return ESP_FAIL;
}
httpd_ws_frame_t ws_capture = {
    .final = true,
    .fragmented = false,
    .len = len,
    .payload = buf, 
    .type = HTTPD_WS_TYPE_BINARY};
    return httpd_ws_send_frame_async(server,client_session_id, &ws_capture);
}


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
    client_session_id = httpd_req_to_sockfd(req);
    if( req->method == HTTP_GET)
        return ESP_OK;

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    ws_pkt.payload = malloc(1024);
    httpd_ws_recv_frame(req, &ws_pkt, 1024);
    ESP_LOGI(TAG, "WS Payload: %.*s\n", ws_pkt.len, ws_pkt.payload);
    free(ws_pkt.payload);

    char* response = "{\"connected\":1, \"packet type\":2}";
    httpd_ws_frame_t ws_response = {
        .final = true,
        .fragmented = false,
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t*)response,
        .len = strlen(response)};
    return httpd_ws_send_frame(req, &ws_response);
    
}

void register_end_points( void ){
    
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
