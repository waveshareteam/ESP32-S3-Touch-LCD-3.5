# Firmware

[English](README.md) | [简体中文](README_ZH.md)

| File | Purpose |
| --- | --- |
| [ESP32-S3-Touch-LCD-3.5_FactoryOnly_20261021.bin](ESP32-S3-Touch-LCD-3.5_FactoryOnly_20261021.bin) | 12-application factory desktop with Button Test, Display Test and backlight enabled after the first frame; ESP-IDF v5.5.4 |

This directory contains the latest factory firmware. Flash the merged image at **0x0**. It includes the bootloader, partition table, application and speech model; camera functions require a connected camera module.

```bash
python -m pip install esptool
python -m esptool --chip esp32s3 --port PORT write_flash 0x0 ESP32-S3-Touch-LCD-3.5_FactoryOnly_20261021.bin
```

Replace `PORT` with your serial port, such as `COMx` or `/dev/ttyACMx`, and run from this directory. Flashing replaces the firmware; retain any device settings you need before restoring it.

SHA256 (20261021): `40f09da5d08c0d2a7b2a2f1cadf3c66bbe3b83f29d09e877395ea3abb2357a94`.

Factory source and build instructions are available in [examples/factory](../examples/factory/).
