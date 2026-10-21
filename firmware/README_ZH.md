# 固件

[English](README.md) | [简体中文](README_ZH.md)

| 文件 | 用途 |
| --- | --- |
| [ESP32-S3-Touch-LCD-3.5_FactoryOnly_20261021.bin](ESP32-S3-Touch-LCD-3.5_FactoryOnly_20261021.bin) | 12 应用出厂桌面，新增按键测试、颜色循环测试，首帧完成后点亮背光；ESP-IDF v5.5.4 |
| [ESP32-S3-Touch-LCD-3.5_FactoryOnly_20261020.bin](ESP32-S3-Touch-LCD-3.5_FactoryOnly_20261020.bin) | V1.0 应用桌面、LVGL 9、AIChats 与摄像头；ESP-IDF v5.5.4 |
| [ESP32-S3-Touch-LCD-3.5_FactoryOnly_20250421.bin](ESP32-S3-Touch-LCD-3.5_FactoryOnly_20250421.bin) | 原有出厂演示；编译日期 2025-04-21，ESP-IDF v5.4 |

这些文件均为从 **0x0** 烧录的合并镜像，按日期或版本选择需要的演示。新版出厂镜像包含引导程序、分区表、应用和 `0x810000` 处的语音模型；摄像头功能需要连接摄像头模块。

```bash
python -m pip install esptool
python -m esptool --chip esp32s3 --port PORT write_flash 0x0 ESP32-S3-Touch-LCD-3.5_FactoryOnly_20261021.bin
```

在本目录运行，将 `PORT` 替换为实际串口，例如 `COMx` 或 `/dev/ttyACMx`。烧录会替换当前固件，恢复前请保留需要的设备配置。

SHA256（20261020）：`cdd9e88e787b90339b9dac8cdcc6c9bded27a865f6968b57487794d2453c7c49`。

SHA256（20250421）：`0a01ded7c9634c49a72fe267da0f10008d88b8bef7645c4cff9decb58a577fc6`。

新旧出厂固件均原样保留。新版出厂源码位于 [examples/factory](../examples/factory/README_ZH.md)，旧版出厂源码位于 [01_factory](../examples/esp-idf/01_factory/README_ZH.md)；新版源码保留原有天气 API 配置。示例固件包发布后可从 [GitHub Releases](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/releases) 下载。
