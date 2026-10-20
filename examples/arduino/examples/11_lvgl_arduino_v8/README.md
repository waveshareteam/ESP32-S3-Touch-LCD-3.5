# 11_lvgl_arduino_v8 — LVGL 8 touch controls

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Displays a greeting and two touch switches using LVGL 8.

## Hardware Required

- ESP32-S3-Touch-LCD-3.5 without enclosure, or ESP32-S3-Touch-LCD-3.5-C with enclosure.
- A USB data cable; prepare the camera, speaker or microSD card when required below.

## How to Run

1. Follow the [development environment](../../../../README.md#development-environment) to install Arduino-ESP32 and make the [bundled libraries](../../libraries/) available in your sketchbook's `libraries` directory.
2. Select **ESP32S3 Dev Module**, Flash **16MB**, PSRAM **OPI PSRAM**, and the remaining board options from the repository README.
3. Open `11_lvgl_arduino_v8.ino` and upload. Set the serial monitor to **115200 baud**.

## Expected Result

The screen shows “Hello Arduino!” and the switches respond to touch.

## Troubleshooting

Use the bundled LVGL 8.4 library and this sketch's `lv_conf.h`. Its APIs are not interchangeable with LVGL 9.
