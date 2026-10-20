# 05_pcf85063_time — RTC serial output

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Initializes PCF85063 and prints its calendar through the serial port.

## Hardware Required

- ESP32-S3-Touch-LCD-3.5 without enclosure, or ESP32-S3-Touch-LCD-3.5-C with enclosure.
- A USB data cable; prepare the camera, speaker or microSD card when required below.

## How to Run

1. Follow the [development environment](../../../../README.md#development-environment) to install Arduino-ESP32 and make the [bundled libraries](../../libraries/) available in your sketchbook's `libraries` directory.
2. Select **ESP32S3 Dev Module**, Flash **16MB**, PSRAM **OPI PSRAM**, and the remaining board options from the repository README.
3. Open `05_pcf85063_time.ino` and upload. Set the serial monitor to **115200 baud**.

## Expected Result

The serial monitor shows the RTC time advancing.

## Troubleshooting

The sketch sets a sample calendar during setup. Edit that value for your test; Wi-Fi time synchronization is not implemented here.
