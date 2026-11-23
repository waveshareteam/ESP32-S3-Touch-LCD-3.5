# Firmware

[English](README.md) | [简体中文](README_ZH.md)

- **Compatible models:** ESP32-S3-Touch-LCD-3.5 (without enclosure) and ESP32-S3-Touch-LCD-3.5-C (with enclosure) share this factory firmware.
- **Camera support:** Supports the **OV5640 camera**. The enclosed model includes the module; the model without an enclosure requires a separately connected camera module for camera functions.
- **Web flashing:** Select **ESP32-S3-Touch-LCD-3.5** in the [firmware center](https://hub.waveshare.net/).

| File | Purpose |
| --- | --- |
| [ESP32-S3-Touch-LCD-3.5_FactoryOnly_20261021.bin](ESP32-S3-Touch-LCD-3.5_FactoryOnly_20261021.bin) | 12-application factory desktop with Button Test, Display Test and backlight enabled after the first frame; ESP-IDF v5.5.4 |

Flash the merged image at **0x0**. It includes the bootloader, partition table, application and speech model.

```bash
python -m pip install esptool
python -m esptool --chip esp32s3 --port PORT write_flash 0x0 ESP32-S3-Touch-LCD-3.5_FactoryOnly_20261021.bin
```

Replace `PORT` with your serial port, such as `COMx` or `/dev/ttyACMx`, and run from this directory. Flashing replaces the firmware; retain any device settings you need before restoring it.

SHA256 (20261021): `40f09da5d08c0d2a7b2a2f1cadf3c66bbe3b83f29d09e877395ea3abb2357a94`.

Factory source and build instructions are available in [examples/factory](../examples/factory/).
