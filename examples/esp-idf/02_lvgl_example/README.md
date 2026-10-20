# 02_lvgl_example — LVGL widgets and touch

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Runs the LVGL 8 widgets demo on the 320 × 480 display.

## Hardware Required

- ESP32-S3-Touch-LCD-3.5 without enclosure, or ESP32-S3-Touch-LCD-3.5-C with enclosure.
- A USB data cable; prepare the camera, speaker or microSD card when required below.

## How to Run

Activate the ESP-IDF environment described in the [development environment](../../../README.md#development-environment), then run from this example's directory:

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p PORT flash monitor
```

Replace `PORT` with your serial port, such as `COMx` on Windows or `/dev/ttyACMx` on Linux.

## Expected Result

The widgets demo appears and responds to touch.

## Troubleshooting

The selected demo is `lv_demo_widgets()` in `main/main.cpp`. Enable the matching LVGL options before selecting another demo.
