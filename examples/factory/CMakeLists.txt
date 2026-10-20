# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(PROJECT_VER "V1.0")
project(esp32_s3_touch_lcd_3_5)

# The product draws its robot face with LVGL objects and uses one Chinese
# text size. Avoid compiling unused image-font widgets and large-font tables.
idf_component_get_property(xiaozhi_font_lib 78__xiaozhi-fonts COMPONENT_LIB)
idf_component_get_property(xiaozhi_font_dir 78__xiaozhi-fonts COMPONENT_DIR)
set_property(TARGET ${xiaozhi_font_lib} PROPERTY SOURCES
    "${xiaozhi_font_dir}/src/font_puhui_16_4.c")
