idf_component_register(SRCS "MicTest.cpp" "assets/img_app_specanalyzer.c"
                       INCLUDE_DIRS "." REQUIRES app_support PRIV_REQUIRES esp_timer)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
