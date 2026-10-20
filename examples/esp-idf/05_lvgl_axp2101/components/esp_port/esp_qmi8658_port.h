#pragma once 
#include "driver/i2c_master.h"
#include "SensorQMI8658.hpp"

extern SensorQMI8658 qmi;

void esp_qmi8658_port_init(i2c_master_bus_handle_t bus_handle);