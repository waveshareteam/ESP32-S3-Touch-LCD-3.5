# 07_sd_card_test — microSD file operations

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Tests SD_MMC mounting, directory operations, file read/write and capacity reporting.

## Hardware Required

- ESP32-S3-Touch-LCD-3.5 without enclosure, or ESP32-S3-Touch-LCD-3.5-C with enclosure.
- A USB data cable; prepare the camera, speaker or microSD card when required below.

## How to Run

1. Follow the [development environment](../../../../README.md#development-environment) to install Arduino-ESP32 and make the [bundled libraries](../../libraries/) available in your sketchbook's `libraries` directory.
2. Select **ESP32S3 Dev Module**, Flash **16MB**, PSRAM **OPI PSRAM**, and the remaining board options from the repository README.
3. Open `07_sd_card_test.ino` and upload. Set the serial monitor to **115200 baud**.

## Expected Result

The serial monitor reports the card type, capacity and file-operation results.

## Troubleshooting

Use a FAT32 test card. The sketch creates, overwrites and deletes test paths including `/mydir`, `/hello.txt`, `/foo.txt` and `/test.txt`; keep personal files with those names off the test card.
