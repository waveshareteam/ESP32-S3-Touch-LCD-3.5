#pragma once

#include "SensorPCF85063.hpp"
#include "driver/i2c_master.h"

extern SensorPCF85063 rtc;

void esp_pcf85063_port_init(i2c_master_bus_handle_t bus_handle);