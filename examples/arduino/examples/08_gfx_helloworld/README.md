# 08_gfx_helloworld — GFX display test

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Initializes ST7796 with Arduino_GFX and draws text on the LCD.

## Hardware Required

- ESP32-S3-Touch-LCD-3.5 without enclosure, or ESP32-S3-Touch-LCD-3.5-C with enclosure.
- A USB data cable; prepare the camera, speaker or microSD card when required below.

## How to Run

1. Follow the [development environment](../../../../README.md#development-environment) to install Arduino-ESP32 and make the [bundled libraries](../../libraries/) available in your sketchbook's `libraries` directory.
2. Select **ESP32S3 Dev Module**, Flash **16MB**, PSRAM **OPI PSRAM**, and the remaining board options from the repository README.
3. Open `08_gfx_helloworld.ino` and upload. Set the serial monitor to **115200 baud**.

## Expected Result

The LCD shows the sketch's text drawing.

## Troubleshooting

Use the bundled Arduino_GFX library and the board settings in the repository README.
