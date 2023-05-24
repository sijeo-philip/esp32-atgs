#include "esp_camera.h"
#include "esp_log.h"

#define CAMERA_TAG  "CAMERA"



#define CAM_PIN_PWDN    32 
#define CAM_PIN_RESET   -1 /*software reset will be performed*/
#define CAM_PIN_XCLK    0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27

#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0       5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

#define CONFIG_XCLK_FREQ 20000000 
#define CONFIG_OV2640_SUPPORT 1
#define CONFIG_OV7725_SUPPORT 1
#define CONFIG_OV3660_SUPPORT 1
#define CONFIG_OV5640_SUPPORT 1


esp_err_t camera_init()
{
    camera_config_t cameraConfig = {
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,

        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,

        .xclk_freq_hz = CONFIG_XCLK_FREQ,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_UXGA,

        .jpeg_quality = 12, 
        .fb_count = 1, 
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY
        
        }; 

        esp_err_t err = esp_camera_init(&cameraConfig);
        if( ESP_OK == err ){
            ESP_LOGI(CAMERA_TAG, "Camera Configured Successfully");
        }
        else 
            ESP_LOGI(CAMERA_TAG, "Camera Configuration Failed: %s\n", esp_err_to_name(err));
    return err;
    
}


esp_err_t camera_capture(camera_fb_t *fb)
{
    ESP_LOGI(CAMERA_TAG, "TAKING PICTURE\n");
    fb = esp_camera_fb_get();
    if (NULL == fb)
    {
            ESP_LOGE(CAMERA_TAG, "CAPTURE FAILED\n");
        return ESP_FAIL;
    }
    return ESP_OK;
}