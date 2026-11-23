# 固件

[English](README.md) | [简体中文](README_ZH.md)

| 文件 | 用途 |
| --- | --- |
| [ESP32-S3-Touch-LCD-3.5_FactoryOnly_20261021.bin](ESP32-S3-Touch-LCD-3.5_FactoryOnly_20261021.bin) | 12 应用出厂桌面，新增按键测试、颜色循环测试，首帧完成后点亮背光；ESP-IDF v5.5.4 |

本目录的最新版出厂固件适用于 ESP32-S3-Touch-LCD-3.5 和带外壳的 ESP32-S3-Touch-LCD-3.5-C。摄像头功能需要连接兼容摄像头模块，带外壳款随附 OV5640 摄像头。

请将合并镜像烧录至 **0x0**，其中已包含引导程序、分区表、应用和语音模型。

```bash
python -m pip install esptool
python -m esptool --chip esp32s3 --port PORT write_flash 0x0 ESP32-S3-Touch-LCD-3.5_FactoryOnly_20261021.bin
```

在本目录运行，将 `PORT` 替换为实际串口，例如 `COMx` 或 `/dev/ttyACMx`。烧录会替换当前固件，恢复前请保留需要的设备配置。

SHA256（20261021）：`40f09da5d08c0d2a7b2a2f1cadf3c66bbe3b83f29d09e877395ea3abb2357a94`。

出厂源码及编译说明请参照 [examples/factory](../examples/factory/README_ZH.md)。
