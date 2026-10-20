
#include "SensorPCF85063.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"

SensorPCF85063 rtc;

void esp_pcf85063_port_init(i2c_master_bus_handle_t bus_handle)
{
    while (!rtc.begin(bus_handle, PCF85063_SLAVE_ADDRESS))
    {
        printf("Failed to find PCF8563 - check your wiring!\r\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    RTC_DateTime datetime = rtc.getDateTime();

    if (datetime.year < 2025)
    {
        datetime.year = 2025;
        datetime.month = 1;
        datetime.day = 1;
        datetime.hour = 12;
        datetime.minute = 0;
        datetime.second = 0;
        rtc.setDateTime(datetime.year, datetime.month, datetime.day, datetime.hour, datetime.minute, datetime.second);
    }
    rtc.start();
}