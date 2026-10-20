# 10_gfx_png — PNG decoding

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Decodes the sample PNG files under `/images/` on the SD card with PNGdec.

## Hardware Required

- ESP32-S3-Touch-LCD-3.5 without enclosure, or ESP32-S3-Touch-LCD-3.5-C with enclosure.
- A USB data cable; prepare the camera, speaker or microSD card when required below.

## How to Run

1. Follow the [development environment](../../../../README.md#development-environment) to install Arduino-ESP32 and make the [bundled libraries](../../libraries/) available in your sketchbook's `libraries` directory.
2. Select **ESP32S3 Dev Module**, Flash **16MB**, PSRAM **OPI PSRAM**, and the remaining board options from the repository README.
3. Open `10_gfx_png.ino` and upload. Set the serial monitor to **115200 baud**.

## Expected Result

The PNG sample images appear on the LCD.

## Troubleshooting

Create an `/images/` directory at the SD card root and copy the contents of `data/` into it. Keep the names `octocat.png` and `octocat-4bpp.png`.
