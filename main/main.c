
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "connect.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "server.h"

#define MAX_APs 20
#define TAG "SCAN"

#if (SHOW_WIFI_SCAN_AP == 1)
static char *getAuthModeName (wifi_auth_mode_t authMode){
    char *names[] = {"OPEN", "WEP", "WPA PSK", "WPA2 PSK", "WPA WPA2 PSK", "MAX"};
    return names[authMode];
}
#endif 

void initialize_nvs(void)
{
  esp_err_t err = nvs_flash_init();
  if (ESP_ERR_NVS_NO_FREE_PAGES == err || ESP_ERR_NVS_NEW_VERSION_FOUND == err) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
  }
}



void app_main(void)
{
  initialize_nvs();
  wifi_init();

#if (SHOW_WIFI_SCAN_AP == 1)
  wifi_scan_config_t scan_config = {
    .ssid = 0,
    .bssid = 0,
    .channel = 0,
    .show_hidden = true,
  };
  ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
  
  wifi_ap_record_t wifiRecords[MAX_APs];
  
  uint16_t maxRecords = MAX_APs;
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&maxRecords, wifiRecords));


  printf("Found %d Access Points:\n", maxRecords);
  printf("\n");
  printf("              SSID                | Channel | RSSI |  Auth Mode \n");
  printf("----------------------------------------------------------------\n");
  for(int i = 0; i < maxRecords; i++ )
    printf("%32s | %7d | %4d | %12s\n", (char*)wifiRecords[i].ssid, wifiRecords[i].primary, wifiRecords[i].rssi, getAuthModeName(wifiRecords[i].authmode));
printf("------------------------------------------------------------------\n");
#endif

esp_err_t err = wifi_connect_sta(10000);
if(err != ESP_OK)
{
   ESP_LOGE(WIFI_TAG, "Failed to Connect \n");
}
start_mdns_service();
register_end_points();
}
