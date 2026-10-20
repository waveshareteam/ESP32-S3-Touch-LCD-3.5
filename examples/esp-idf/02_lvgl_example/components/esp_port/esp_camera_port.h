#pragma once

#include "esp_camera.h"
#include "driver/i2c_master.h"

void esp_camera_port_init(i2c_port_num_t i2c_port);