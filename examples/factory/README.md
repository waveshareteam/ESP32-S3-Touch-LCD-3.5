# factory — Application desktop

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Runs a 320 × 480 portrait desktop with Settings, AIChats, Touch Test, Photos, Weather, Music, Sensor, 2048, Camera and Mic Test. The BSP initializes the board peripherals; applications use LVGL 9.6 and the Brookesia lifecycle.

## Hardware Required

Use ESP32-S3-Touch-LCD-3.5 or ESP32-S3-Touch-LCD-3.5-C and a USB data cable. Camera, audio and SD functions require a compatible camera, speaker and FAT32 microSD card respectively; the enclosed version includes an OV5640 camera.

## How to Run

Use the factory ESP-IDF version in the [development environment](../../README.md#development-environment). Run `idf.py set-target esp32s3`, `idf.py build` and `idf.py -p PORT flash monitor`; the original weather API settings are included, and can be changed with the location under `idf.py menuconfig` → **S3 3.5 Weather**.

## Expected Result

The desktop opens with Settings first and AIChats second; tap an icon to open an application. Settings provides brightness, volume, Wi-Fi scanning/setup and SD information; place JPEG photos in `/photo` and MP3 audio in `/music` at the SD-card root.

For application controls and Wi-Fi setup, see the [user guide](USER_GUIDE.md).

## Troubleshooting

Wi-Fi uses saved credentials first, otherwise the demonstration network configured in [wifi_manager.h](components/network_service/wifi_manager.h); connect another network through **Settings → Wi-Fi setup**, scan the displayed QR code or join its hotspot, and open `http://192.168.4.1`. Weather uses the included API settings for `cn.apihz.cn`; AIChats requires network activation, and a full `idf.py flash` is needed to install the speech model at `0x810000` as well as the application.
