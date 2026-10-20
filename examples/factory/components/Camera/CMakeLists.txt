idf_component_register(SRCS "Camera.cpp" "assets/img_app_camera.c" INCLUDE_DIRS "." REQUIRES app_support PRIV_REQUIRES esp32-camera esp_timer)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
