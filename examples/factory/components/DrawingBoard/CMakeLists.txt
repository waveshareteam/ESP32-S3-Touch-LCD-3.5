idf_component_register(SRCS "DrawingBoard.cpp" "assets/image_draw.c"
    INCLUDE_DIRS "." REQUIRES brookesia_core lvgl esp32_s3_touch_lcd_3_5)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
