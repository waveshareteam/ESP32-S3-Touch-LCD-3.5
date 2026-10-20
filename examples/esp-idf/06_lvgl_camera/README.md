# 06_lvgl_camera — Camera preview

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Shows camera frames on the LCD using the bundled camera driver and LVGL UI.

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

The camera page displays the connected camera's preview.

## Troubleshooting

A camera module is required. The -C product includes an OV5640; the bare-board version requires a separate compatible module. Check the camera cable and initialization log if no frame appears.
