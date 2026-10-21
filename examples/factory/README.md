# Factory application suite

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Provides a 320 × 480 portrait desktop with 12 applications: Settings, AIChats, Touch Test, Photos, Weather, Music, Sensor, 2048, Camera, Mic Test, Button Test and Display Test.

## Hardware Required

Use ESP32-S3-Touch-LCD-3.5 or ESP32-S3-Touch-LCD-3.5-C and a USB data cable. Camera, audio and SD functions require a compatible camera, speaker and FAT32 microSD card respectively; the enclosed version includes an OV5640 camera.

## How to Run

To restore the factory software, follow the [firmware instructions](../../firmware/README.md). To build from source, use ESP-IDF v5.5.4 as described in the [development environment](../../README.md#development-environment), then run `idf.py set-target esp32s3`, `idf.py build` and `idf.py -p PORT flash monitor` from this directory; replace `PORT` with the board's serial port.

## Expected Result

The desktop opens with 12 applications, with Settings first and AIChats second; tap an icon to open an application. The screen lights up when the desktop is ready. Settings provides brightness, volume, Wi-Fi scanning/setup and SD information; place JPEG photos in `/photo` and MP3 audio in `/music` at the SD-card root.

Short press PWR once to power on; press and hold PWR for 5 seconds to power off. Use short presses when checking PWR in Button Test.

For application controls and Wi-Fi setup, see the [user guide](USER_GUIDE.md).

## Troubleshooting

Weather and AIChats require Internet access; complete AIChats activation using the instructions on screen. If a speech model is missing, flash the complete factory image or run a full `idf.py flash` rather than flashing only the application; see the [user guide](USER_GUIDE.md#common-questions) for other common issues.
