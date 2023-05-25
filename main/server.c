
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "mdns.h"
#include "esp_wifi.h"
#include "esp_camera.h"
#include "nvs_flash.h"
#include "connect.h"


#define TAG     "SERVER"


static int client_session_id;
static httpd_handle_t server = NULL;


void reset_wifi( void* params)
{
    vTaskDelay(1000/portTICK_PERIOD_MS);
    esp_wifi_stop();
    xSemaphoreGive(initSemaphore);
    vTaskDelete(NULL);
}

esp_err_t send_camera_data_ws(const char* buf, size_t len){
if(!client_session_id)
{
    ESP_LOGE(TAG, "no client session id");
    return ESP_FAIL;
}
httpd_ws_frame_t ws_capture = {
    .final = true,
    .fragmented = false,
    .len = len,
    .payload =(uint8_t*)buf, 
    .type = HTTPD_WS_TYPE_TEXT};
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
    return 0;
}

static esp_err_t on_camera_capture(httpd_req_t *req)
{

    ESP_LOGI("CAMERA", "TAKING PICTURE\n");
    camera_fb_t *fb = esp_camera_fb_get();
    if (NULL == fb)
    {
        ESP_LOGE("CAMERA", "CAPTURE FAILED\n");
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    char* num = malloc(10);
    httpd_resp_set_hdr(req, "Content-Length", itoa(fb->len, num, 10));
    esp_err_t err = httpd_resp_send(req, (char*)fb->buf, fb->len);
    free(num);
    ESP_LOGI("CAMERA", "%s", esp_err_to_name(err));
    esp_camera_fb_return(fb);
    ESP_LOGI("CAMERA", "TAKEN PICTURE\n");
    
    return ESP_OK;
}

static esp_err_t on_post_wifi_config(httpd_req_t *req)
{
    ESP_LOGI(TAG, "URL %s was hit", req->uri);
    char* buf = malloc(150);
    memset(buf, '\0', sizeof(150));
    wifi_config_t *wifiConfigSet = malloc(sizeof(wifi_config_t));
    httpd_req_recv(req, buf, req->content_len);
    cJSON *payload = cJSON_Parse(buf);
    cJSON *ssid = cJSON_GetObjectItem(payload, "ssid");
    if( cJSON_IsString(ssid) && (ssid->valuestring !=NULL))
    {
        strncpy((char*)wifiConfigSet->sta.ssid, ssid->valuestring, strlen(ssid->valuestring));
        ESP_LOGI(TAG, "SSID : %s", ssid->string);
    }
    cJSON *password = cJSON_GetObjectItem(payload, "pass");
    if( cJSON_IsString(password) && ( password->valuestring != NULL) )
    {
        strncpy((char*)wifiConfigSet->sta.password, password->valuestring, strlen(password->valuestring));
        ESP_LOGI(TAG, "PASSWORD: %s", password->valuestring);
    }
    wifiConfigSet->sta.threshold.authmode = DEFAULT_AUTHMODE;
    free(buf);
    cJSON_Delete(payload);
    nvs_handle_t wifiCredNVSHandle;
    //Open the NVS
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &wifiCredNVSHandle);
    if (ESP_OK != err)
    {
        ESP_LOGI(NVS_TAG, "Could not Open NVS: %s\n", STORAGE_NAMESPACE);
        return err;
    }
    err = nvs_set_blob(wifiCredNVSHandle, "wifiCred", (void*)wifiConfigSet, sizeof(wifi_config_t)+1);
    if(ESP_OK != err)
    {
        ESP_LOGI(NVS_TAG, "Could not Write WifiCred to NVS: %s\n",esp_err_to_name(err));
    }
    free(wifiConfigSet);
    httpd_resp_set_status(req, "200");
    httpd_resp_send(req, NULL, 0);
    xTaskCreate(reset_wifi, "reset wifi", 1024*2, NULL, 15, NULL);
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

    httpd_uri_t cameraCapture = {
        .uri = "/capture", 
        .method = HTTP_GET, 
        .handler = on_camera_capture,
    };
    httpd_register_uri_handler(server, &cameraCapture);
}

void start_mdns_service (void)
{
    mdns_init();
    mdns_hostname_set("targetsystem");
    mdns_instance_name_set("LEARN ESP32 MDNS");
}
