# 04_lvgl_qmi8658 — QMI8658 motion display

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Reads the onboard QMI8658 accelerometer and gyroscope and displays the measurements in LVGL.

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

Moving or rotating the board changes the displayed motion readings.

## Troubleshooting

Keep the board still when checking the zero-rate readings. If initialization fails, inspect the I2C error log and use the schematic for your board revision.
