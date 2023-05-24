
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
#include "camera.h"
#include "driver/gpio.h"
#include <string.h>

#define MAX_APs 20
#define TAG "SCAN"

#define BUTTON_PIN 3

static esp_err_t button_init(void);
static void button_handler (void *arg);
static void take_photo(void *arg);

static esp_err_t button_init( void ){
  gpio_config_t ioConf;

  ioConf.mode = GPIO_MODE_INPUT;
  ioConf.pin_bit_mask = (1UL << BUTTON_PIN );
  ioConf.intr_type = GPIO_INTR_POSEDGE;
  ioConf.pull_up_en = 1;
  esp_err_t err = gpio_config(&ioConf);
  if( err != ESP_OK)
  {
    ESP_LOGI("GPIO", "ERROR configuring GPIO\n");
    return err;
  }
  gpio_install_isr_service(0);
  return gpio_isr_handler_add(BUTTON_PIN, button_handler, NULL);
}


/************************************************/
static TickType_t next = 0;
const TickType_t period = 2000 / portTICK_PERIOD_MS;

static void IRAM_ATTR button_handler(void *arg)
{
    TickType_t now = xTaskGetTickCountFromISR();

    if (now > next)
    {
        xTaskCreate(take_photo, "pic", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);
    }
    next = now + period;
}

static void take_photo(void *arg)
{
  #if 0
    ESP_LOGI("CAMERA", "Taking Picture\n");
    camera_fb_t *pic = NULL;
    static size_t picSize = 0;
    ESP_ERROR_CHECK(camera_capture(pic));
    picSize = pic->len;
    uint8_t* cam_buff = malloc(picSize+1);
    memcpy(cam_buff, pic->buf, pic->len);
    esp_camera_fb_return(pic);
    send_camera_data_ws(cam_buff, picSize);
    free(cam_buff);
    ESP_LOGI("CAMERA", "Finished Taking Picture!\n");
    #endif
    ESP_LOGI("CAMERA", "TAKING PICTURE\n");
    camera_fb_t *fb = esp_camera_fb_get();
    if (NULL == fb)
    {
     ESP_LOGE("CAMERA", "CAPTURE FAILED\n");
    }
    esp_err_t err = send_camera_data_ws(fb->buf, fb->len);
    ESP_LOGI("CAMERA", "%s", esp_err_to_name(err));
    esp_camera_fb_return(fb);
    ESP_LOGI("CAMERA", "TAKEN PICTURE\n");
    vTaskDelete(NULL);
    
}
/************************************************/
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
button_init();
camera_init();
start_mdns_service();
register_end_points();

}
