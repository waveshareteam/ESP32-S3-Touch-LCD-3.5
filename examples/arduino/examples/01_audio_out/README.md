# 01_audio_out — SD MP3 playback

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Plays `/1.mp3` from the microSD card through ES8311 and the speaker.

## Hardware Required

- ESP32-S3-Touch-LCD-3.5 without enclosure, or ESP32-S3-Touch-LCD-3.5-C with enclosure.
- A USB data cable; prepare the camera, speaker or microSD card when required below.

## How to Run

1. Follow the [development environment](../../../../README.md#development-environment) to install Arduino-ESP32 and make the [bundled libraries](../../libraries/) available in your sketchbook's `libraries` directory.
2. Select **ESP32S3 Dev Module**, Flash **16MB**, PSRAM **OPI PSRAM**, and the remaining board options from the repository README.
3. Open `01_audio_out.ino` and upload. Set the serial monitor to **115200 baud**.

## Expected Result

Audio plays through the connected speaker.

## Troubleshooting

Copy `data/1.mp3` to the root of a FAT32 microSD card before starting. Keep OPI PSRAM enabled.
