idf_component_register(SRCS "main.cpp" "desktop.cpp" "assets/img_desktop_wallpaper.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp32_s3_touch_lcd_3_5 brookesia_core Settings DrawingBoard Photos Weather Music Sensor Games Camera XiaozhiApp MicTest network_service)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
