# 08_lvgl_image — SD-card image viewer

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Lists .bin, .jpg, .jpeg and .png files under the SD card's /images folder and displays them with LVGL.

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

The first image is displayed; horizontal swipe gestures select other images.

## Troubleshooting

Before boot, copy the contents of `data/` to `/images/` on a FAT32 microSD card. Keep at least one supported image in that folder. `.bin` files must be LVGL 8 image binaries matching this example's color configuration, not firmware images.
