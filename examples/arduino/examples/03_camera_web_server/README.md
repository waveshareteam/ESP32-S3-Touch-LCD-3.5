# 03_camera_web_server — Camera web server

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Connects to Wi-Fi and serves the camera through a browser interface.

## Hardware Required

- ESP32-S3-Touch-LCD-3.5 without enclosure, or ESP32-S3-Touch-LCD-3.5-C with enclosure.
- A USB data cable; prepare the camera, speaker or microSD card when required below.

## How to Run

1. Follow the [development environment](../../../../README.md#development-environment) to install Arduino-ESP32 and make the [bundled libraries](../../libraries/) available in your sketchbook's `libraries` directory.
2. Select **ESP32S3 Dev Module**, Flash **16MB**, PSRAM **OPI PSRAM**, and the remaining board options from the repository README.
3. Open `03_camera_web_server.ino` and upload. Set the serial monitor to **115200 baud**.

## Expected Result

The serial monitor prints the camera URL; a browser on the same network can open it.

## Troubleshooting

Edit `ssid` and `password` in the sketch before uploading. A compatible camera module is required; the -C product includes an OV5640.
