#ifndef SERVER_H
#define SERVER_H

#include "esp_err.h"

void register_end_points(void);
void start_mdns_service (void);
esp_err_t send_camera_data_ws(const char* buf, size_t len);

#endif