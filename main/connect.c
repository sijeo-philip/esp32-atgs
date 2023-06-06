#include <stdio.h>
#include <string.h>
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "connect.h"




static EventGroupHandle_t wifiEvents;
const int CONNECTED_GOT_IP = BIT0;
const int DISCONNECTED = BIT1;

static esp_netif_t *sta_netif;
static esp_netif_t *ap_netif;
static bool connection_status = false;
static uint8_t disconnect_count = 0;

extern SemaphoreHandle_t connectionSemaphore;
extern httpd_handle_t server;

static char *print_disconnection_error(wifi_err_reason_t reason)
{
    switch (reason)
    {
    case WIFI_REASON_UNSPECIFIED:
        return "WIFI_REASON_UNSPECIFIED";
    case WIFI_REASON_AUTH_EXPIRE:
        return "WIFI_REASON_AUTH_EXPIRE";
    case WIFI_REASON_AUTH_LEAVE:
        return "WIFI_REASON_AUTH_LEAVE";
    case WIFI_REASON_ASSOC_EXPIRE:
        return "WIFI_REASON_ASSOC_EXPIRE";
    case WIFI_REASON_ASSOC_TOOMANY:
        return "WIFI_REASON_ASSOC_TOOMANY";
    case WIFI_REASON_NOT_AUTHED:
        return "WIFI_REASON_NOT_AUTHED";
    case WIFI_REASON_NOT_ASSOCED:
        return "WIFI_REASON_NOT_ASSOCED";
    case WIFI_REASON_ASSOC_LEAVE:
        return "WIFI_REASON_ASSOC_LEAVE";
    case WIFI_REASON_ASSOC_NOT_AUTHED:
        return "WIFI_REASON_ASSOC_NOT_AUTHED";
    case WIFI_REASON_DISASSOC_PWRCAP_BAD:
        return "WIFI_REASON_DISASSOC_PWRCAP_BAD";
    case WIFI_REASON_DISASSOC_SUPCHAN_BAD:
        return "WIFI_REASON_DISASSOC_SUPCHAN_BAD";
    case WIFI_REASON_IE_INVALID:
        return "WIFI_REASON_IE_INVALID";
    case WIFI_REASON_MIC_FAILURE:
        return "WIFI_REASON_MIC_FAILURE";
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
        return "WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT";
    case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT:
        return "WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT";
    case WIFI_REASON_IE_IN_4WAY_DIFFERS:
        return "WIFI_REASON_IE_IN_4WAY_DIFFERS";
    case WIFI_REASON_GROUP_CIPHER_INVALID:
        return "WIFI_REASON_GROUP_CIPHER_INVALID";
    case WIFI_REASON_PAIRWISE_CIPHER_INVALID:
        return "WIFI_REASON_PAIRWISE_CIPHER_INVALID";
    case WIFI_REASON_AKMP_INVALID:
        return "WIFI_REASON_AKMP_INVALID";
    case WIFI_REASON_UNSUPP_RSN_IE_VERSION:
        return "WIFI_REASON_UNSUPP_RSN_IE_VERSION";
    case WIFI_REASON_INVALID_RSN_IE_CAP:
        return "WIFI_REASON_INVALID_RSN_IE_CAP";
    case WIFI_REASON_802_1X_AUTH_FAILED:
        return "WIFI_REASON_802_1X_AUTH_FAILED";
    case WIFI_REASON_CIPHER_SUITE_REJECTED:
        return "WIFI_REASON_CIPHER_SUITE_REJECTED";
    case WIFI_REASON_INVALID_PMKID:
        return "WIFI_REASON_INVALID_PMKID";
    case WIFI_REASON_BEACON_TIMEOUT:
        return "WIFI_REASON_BEACON_TIMEOUT";
    case WIFI_REASON_NO_AP_FOUND:
        return "WIFI_REASON_NO_AP_FOUND";
    case WIFI_REASON_AUTH_FAIL:
        return "WIFI_REASON_AUTH_FAIL";
    case WIFI_REASON_ASSOC_FAIL:
        return "WIFI_REASON_ASSOC_FAIL";
    case WIFI_REASON_HANDSHAKE_TIMEOUT:
        return "WIFI_REASON_HANDSHAKE_TIMEOUT";
    case WIFI_REASON_CONNECTION_FAIL:
        return "WIFI_REASON_CONNECTION_FAIL";
    case WIFI_REASON_AP_TSF_RESET:
        return "WIFI_REASON_AP_TSF_RESET";
    case WIFI_REASON_ROAMING:
        return "WIFI_REASON_ROAMING";
    default:
        return "OTHER ERROR";
    }

    return "";
}

static void event_handler(void* arg, esp_event_base_t eventBase, int32_t eventID, void* eventData){
  if(eventBase == WIFI_EVENT)
    {
        switch (eventID)
        {
            case WIFI_EVENT_STA_START:
                ESP_LOGI("WIFI", "Connecting... \n");
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI("WIFI", "Connected\n");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                wifi_event_sta_disconnected_t *wifi_event_sta_disconnected = eventData;
                //from wifi_err_reason_t
                ESP_LOGW("WIFI", "Disconnected code %d: %s\n", wifi_event_sta_disconnected->reason,
                                 print_disconnection_error(wifi_event_sta_disconnected->reason));
                disconnect_count = disconnect_count + 1;
                ESP_LOGI("CONNECT", "Disconnects in count %d", disconnect_count);
                if(disconnect_count > 5)
                {
                    ESP_ERROR_CHECK(nvs_flash_erase());
                    wifi_disconnect();
                    disconnect_count = 0;
                    xSemaphoreGiveFromISR(initSemaphore, pdFALSE);
                }
                else
                {
                    esp_wifi_connect();
                    break;
                }
                xEventGroupSetBits(wifiEvents, DISCONNECTED);
                break;
            default:
            break;
    
        }
    }
    if (eventBase == IP_EVENT)
    {
        switch (eventID)
        {
        case IP_EVENT_STA_GOT_IP:
            ESP_LOGI("WIFI","GOT IP\n");
            xEventGroupSetBits(wifiEvents, CONNECTED_GOT_IP);
            break;
        
        default:
            break;
        }
    }
    
}

#if 0
static esp_err_t get_wifi_creds (wifi_config_t *p_WifiConfig)
{
    esp_err_t err;
    nvs_handle_t wifiCredNVSHandle;
    //Open the NVS
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &wifiCredNVSHandle);
    if (ESP_OK != err)
    {
        ESP_LOGI(NVS_TAG, "Could not Open NVS: %s\n", STORAGE_NAMESPACE);
        return err;
    }
    size_t wifiCredSize = 0;
    
    err = nvs_get_blob(wifiCredNVSHandle, "wifiCred", NULL, &wifiCredSize);
    if( err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
    wifi_config_t *wifiConfigSet = malloc(wifiCredSize + sizeof(wifi_config_t));
    memset(wifiConfigSet, '\0', wifiCredSize+sizeof(wifi_config_t));
    if ( wifiCredSize > 0 )
    {
        err = nvs_get_blob(wifiCredNVSHandle, "wifiCred", wifiConfigSet, &wifiCredSize);
        if (ESP_OK != err )
        {
            free(wifiConfigSet);
            return err;
        }
        else
        {
            strncpy((char*)p_WifiConfig->sta.ssid, (char*)wifiConfigSet->sta.ssid, sizeof(wifiConfigSet->sta.ssid));
            strncpy((char*)p_WifiConfig->sta.password, (char*)wifiConfigSet->sta.password, sizeof(wifiConfigSet->sta.password));
            p_WifiConfig->sta.threshold.authmode = wifiConfigSet->sta.threshold.authmode;
            free(wifiConfigSet);
        }
    }
     if ( ESP_ERR_NVS_NOT_FOUND == err )
    {
        memset(p_WifiConfig, '\0', sizeof(wifi_config_t));
        strncpy((char*)p_WifiConfig->sta.ssid, (char*)defaultWifiConfig.sta.ssid, sizeof(defaultWifiConfig.sta.ssid));
        strncpy((char*)p_WifiConfig->sta.password, (char*)defaultWifiConfig.sta.password, sizeof(defaultWifiConfig.sta.password));
        p_WifiConfig->sta.threshold.authmode = defaultWifiConfig.sta.threshold.authmode;
        err = nvs_set_blob(wifiCredNVSHandle, "wifiCred", (void*)p_WifiConfig, sizeof(wifi_config_t)+1);
        if(ESP_OK != err)
        {
            ESP_LOGI(NVS_TAG, "Could not Write WifiCred to NVS: %s\n",esp_err_to_name(err));
            return err;
        }
    }
    else if (ESP_OK != err)
    {
        ESP_LOGI(NVS_TAG, "Could not Read WifiCred from NVS: %s, Read Length: %d\n", esp_err_to_name(err), wifiCredSize);
        ESP_ERROR_CHECK(nvs_flash_erase());
    }
    nvs_close(wifiCredNVSHandle);
    ESP_LOGI(WIFI_TAG, "ssid: %s, password: %s, total bytes read: %d\n", p_WifiConfig->sta.ssid, p_WifiConfig->sta.password, wifiCredSize);
    return err;
}
#endif


esp_err_t wifi_connect_sta( wifi_config_t *p_wifiConfig){
    wifiEvents = xEventGroupCreate();
    esp_err_t err = ESP_OK;
/*Initialize default station as network interface instance*/
    sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if ( ESP_OK != err )
        return err;
    ESP_LOGI(WIFI_TAG, "Setting Wifi IF STA \n");
    wifi_config_t *set_wifi_config= malloc(sizeof(wifi_config_t));
    memset(set_wifi_config, 0, sizeof(wifi_config_t));
    strncpy((char*)set_wifi_config->sta.ssid, (char*)p_wifiConfig->sta.ssid, sizeof(p_wifiConfig->sta.ssid));
    strncpy((char*)set_wifi_config->sta.password, (char*)p_wifiConfig->sta.password, sizeof(p_wifiConfig->sta.password));
    set_wifi_config->sta.threshold.authmode =  p_wifiConfig->sta.threshold.authmode;
        
    err = esp_wifi_set_config(WIFI_IF_STA, set_wifi_config);
    free(set_wifi_config);
       
   
return err;
}

void wifi_connect_ap( const char* ssid, const char* pass ){
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));
    strncpy((char *)wifi_config.ap.ssid, ssid, strlen(ssid));
    strncpy((char *)wifi_config.ap.password, pass, strlen(pass));
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.beacon_interval = 100;

    ap_netif = esp_netif_create_default_wifi_ap();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
}

void wifi_disconnect( void ){
    ESP_LOGI(WIFI_TAG, "**********DISCONNECTING*********");
    esp_wifi_disconnect();
    httpd_stop(server);
    esp_wifi_stop();
    ESP_LOGI(WIFI_TAG, "***********DISCONNECTING COMPLETE*********");

}


void wifi_init(void *params)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    while(true)
    {
        if(xSemaphoreTake(initSemaphore, portMAX_DELAY))
        {
            esp_err_t err;
            nvs_handle_t wifiCredNVSHandle;
            //Open the NVS
            err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &wifiCredNVSHandle);
            if (ESP_OK == err)
            {
                size_t wifiCredSize = 0;
    
                err = nvs_get_blob(wifiCredNVSHandle, "wifiCred", NULL, &wifiCredSize);
                if( err != ESP_OK )
                {
                    nvs_close(wifiCredNVSHandle);
                    connection_status = false;
                }
                wifi_config_t *wifiConfigSet = malloc(wifiCredSize + sizeof(wifi_config_t));
                memset(wifiConfigSet, '\0', wifiCredSize+sizeof(wifi_config_t));
                if ( wifiCredSize > 0 )
                {
                    err = nvs_get_blob(wifiCredNVSHandle, "wifiCred", wifiConfigSet, &wifiCredSize);
                    if (ESP_OK != err )
                    {
                        free(wifiConfigSet);
                        nvs_close(wifiCredNVSHandle);
                        connection_status = false;
                    }
                    else
                    {
                        connection_status = true;
                        nvs_close(wifiCredNVSHandle);
                        wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
                        ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
                        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
                        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
                        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
                        err = wifi_connect_sta( wifiConfigSet );
                        free(wifiConfigSet);
                    }
                }

            }
            
            if( ( connection_status == true) && (err == ESP_OK) )
            {
                ESP_ERROR_CHECK(esp_wifi_start());

            }
            else{
                wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
                ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
                wifi_connect_ap("TARGET_SYSTEM","nopassword");
                ESP_ERROR_CHECK(esp_wifi_start());
            }
            xSemaphoreGive(connectionSemaphore);        
        }
    }
}