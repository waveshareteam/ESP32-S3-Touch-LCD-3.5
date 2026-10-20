idf_component_register(
    SRCS "esp32_s3_touch_lcd_3_5.cpp" "bsp_audio.cpp"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_lcd esp_lcd_st7796 esp_lcd_touch_ft6336
             esp_lvgl_adapter lvgl esp_codec_dev esp32-camera
             fatfs sdmmc esp_wifi esp_netif
    PRIV_REQUIRES XPowersLib sensorlib esp_io_expander_tca9554
                  button nvs_flash esp_timer esp_event)
