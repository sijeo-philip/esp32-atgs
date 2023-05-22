#ifndef CONNECT_H
#define CONNECT_H



#include "esp_err.h"

#define NVS_TAG     "NVS"
#define WIFI_TAG    "WIFI"

/**Set the below MACRO to 1 to print all the WIFI scanned APs*/
#ifndef SHOW_WIFI_SCAN_AP
#define SHOW_WIFI_SCAN_AP       0
#endif

#ifndef DEFAULT_SSID
#define DEFAULT_SSID   "Airtel_9902330851"
#endif

#ifndef DEFAULT_PWD     
#define DEFAULT_PWD     "air11964"
#endif 

#ifndef DEFAULT_SCAN_METHOD
#define DEFAULT_SCAN_METHOD     WIFI_FAST_SCAN     /*Possible Value is WIFI_FAST_SCAN/WIFI_ALL_CHANNEL_SCAN*/
#endif

#ifndef DEFAULT_SORT_METHOD     
#define DEFAULT_SORT_METHOD     WIFI_CONNECT_AP_BY_SECURITY
#endif

#ifndef DEFAULT_RSSI
#define DEFAULT_RSSI        -127
#endif

#ifndef DEFAULT_AUTHMODE
#define DEFAULT_AUTHMODE    WIFI_AUTH_WPA_WPA2_PSK
#endif 

#ifndef STORAGE_NAMESPACE
#define STORAGE_NAMESPACE   "storage"
#endif

void wifi_init( void );
esp_err_t wifi_connect_sta( int timeout );
void wifi_connect_ap( const char* ssid, const char* pass );
void wifi_disconnect( void );

#endif