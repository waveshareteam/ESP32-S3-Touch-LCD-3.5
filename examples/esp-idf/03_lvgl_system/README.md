# 03_lvgl_system — System information, RTC and audio controls

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Shows the system page with RTC, storage information and audio/display controls.

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

The system page shows peripheral information; controls use the initialized audio and display interfaces.

## Troubleshooting

Insert a FAT32 microSD card for storage information and connect a speaker for audio output. RTC initialization uses the example's calendar settings; it is not Internet time synchronization.
