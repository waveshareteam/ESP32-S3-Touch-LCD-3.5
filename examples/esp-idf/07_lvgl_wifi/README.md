# 07_lvgl_wifi — Wi-Fi connection and scan UI

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Initializes Wi-Fi and shows the LVGL Wi-Fi page.

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

The Wi-Fi page shows scan results and the connection state.

## Troubleshooting

Set the SSID and password passed to `esp_wifi_port_init()` in `main/main.cpp` to your own 2.4 GHz network. The supplied credentials are demonstration values.
