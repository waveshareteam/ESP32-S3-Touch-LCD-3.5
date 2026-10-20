# 05_lvgl_axp2101 — AXP2101 power telemetry

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Displays AXP2101 power-management measurements in an LVGL page.

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

The screen shows the available power and battery telemetry.

## Troubleshooting

Battery readings require a compatible battery to be connected. Keep the original power-rail settings unless you have checked the schematic.
