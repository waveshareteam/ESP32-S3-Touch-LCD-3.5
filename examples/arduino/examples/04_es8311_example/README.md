# 04_es8311_example — Microphone record and replay

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Records five seconds of microphone audio into RAM, then plays the recorded WAV through I2S and ES8311.

## Hardware Required

- ESP32-S3-Touch-LCD-3.5 without enclosure, or ESP32-S3-Touch-LCD-3.5-C with enclosure.
- A USB data cable; prepare the camera, speaker or microSD card when required below.

## How to Run

1. Follow the [development environment](../../../../README.md#development-environment) to install Arduino-ESP32 and make the [bundled libraries](../../libraries/) available in your sketchbook's `libraries` directory.
2. Select **ESP32S3 Dev Module**, Flash **16MB**, PSRAM **OPI PSRAM**, and the remaining board options from the repository README.
3. Open `04_es8311_example.ino` and upload. Set the serial monitor to **115200 baud**.

## Expected Result

Speak after reset; the recording is replayed through the speaker.

## Troubleshooting

Connect a speaker. This is a RAM recording example; it does not save a WAV file to the SD card.
