# Firmware

[English](README.md) | [简体中文](README_ZH.md)

| File | Purpose |
| --- | --- |
| [ESP32-S3-Touch-LCD-3.5_FactoryOnly_20261020.bin](ESP32-S3-Touch-LCD-3.5_FactoryOnly_20261020.bin) | V1.0 application desktop, LVGL 9, AIChats and camera; ESP-IDF v5.5.4 |
| [ESP32-S3-Touch-LCD-3.5_FactoryOnly_20250421.bin](ESP32-S3-Touch-LCD-3.5_FactoryOnly_20250421.bin) | Original factory demonstration; build date 2025-04-21, ESP-IDF v5.4 |

These files are merged images flashed at **0x0**; select the date or version matching the desired demonstration. The current factory image contains the bootloader, partition table, application and speech model at `0x810000`; camera functions require a connected camera module.

```bash
python -m pip install esptool
python -m esptool --chip esp32s3 --port PORT write_flash 0x0 ESP32-S3-Touch-LCD-3.5_FactoryOnly_20261020.bin
```

Replace `PORT` with your serial port, such as `COMx` or `/dev/ttyACMx`, and run from this directory. Flashing replaces the firmware; retain any device settings you need before restoring it.

SHA256 (20261020): `cdd9e88e787b90339b9dac8cdcc6c9bded27a865f6968b57487794d2453c7c49`.

SHA256 (20250421): `0a01ded7c9634c49a72fe267da0f10008d88b8bef7645c4cff9decb58a577fc6`.

The supplied binaries are retained byte-for-byte. Current source is in [examples/factory](../examples/factory/), and legacy source is in [01_factory](../examples/esp-idf/01_factory/); the current source retains the original weather API settings. Example firmware packages are available from [GitHub Releases](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/releases) when published.
