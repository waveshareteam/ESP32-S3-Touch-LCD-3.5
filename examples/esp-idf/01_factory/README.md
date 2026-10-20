# 01_factory — Legacy factory demonstration

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Initializes the display, touch, PMU, audio, IMU, RTC, SD card, camera and Wi-Fi, then runs the original LVGL 8 factory demonstration. The current application desktop is maintained separately in [factory](../../factory/).

## Hardware Required

Use ESP32-S3-Touch-LCD-3.5 or ESP32-S3-Touch-LCD-3.5-C with a USB data cable. Connect a compatible camera and speaker and insert a FAT32 microSD card to exercise those peripherals.

## How to Run

Activate the ESP-IDF environment in the [development environment](../../../README.md#development-environment), then run `idf.py set-target esp32s3`, `idf.py build` and `idf.py -p PORT flash monitor` from this directory. Replace `PORT` with your serial port.

## Expected Result

Startup enters a touch test. Press BOOT once to finish the test and open the legacy demonstration UI.

## Troubleshooting

Set the Wi-Fi credentials in `main/main.cpp`. Optional peripheral failures are reported in the serial log; this example retains its original LVGL 8 dependencies independently of the current factory desktop.
