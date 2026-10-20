# 02_axp2101_example — Power-management test

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Initializes AXP2101 and prints power/battery information to the serial monitor.

## Hardware Required

- ESP32-S3-Touch-LCD-3.5 without enclosure, or ESP32-S3-Touch-LCD-3.5-C with enclosure.
- A USB data cable; prepare the camera, speaker or microSD card when required below.

## How to Run

1. Follow the [development environment](../../../../README.md#development-environment) to install Arduino-ESP32 and make the [bundled libraries](../../libraries/) available in your sketchbook's `libraries` directory.
2. Select **ESP32S3 Dev Module**, Flash **16MB**, PSRAM **OPI PSRAM**, and the remaining board options from the repository README.
3. Open `02_axp2101_example.ino` and upload. Set the serial monitor to **115200 baud**.

## Expected Result

The serial monitor reports PMU information.

## Troubleshooting

A battery must be connected for battery readings. Do not change rail voltages without checking the board schematic.
