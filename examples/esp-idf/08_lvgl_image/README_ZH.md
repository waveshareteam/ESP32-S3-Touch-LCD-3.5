# 08_lvgl_image — SD 卡图片浏览

[English](README.md) | [简体中文](README_ZH.md)

## 功能

枚举 SD 卡 /images 目录下的 .bin、.jpg、.jpeg 和 .png 文件，并通过 LVGL 显示。

## 硬件需求

- ESP32-S3-Touch-LCD-3.5 不带外壳版，或 ESP32-S3-Touch-LCD-3.5-C 带外壳版。
- USB 数据线；按下方说明准备摄像头、扬声器或 microSD 卡等所需外设。

## 怎么运行

根据[开发环境](../../../README_ZH.md#development-environment)启用 ESP-IDF 环境，在本示例目录执行：

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p PORT flash monitor
```

将 `PORT` 替换为实际串口，例如 Windows 的 `COMx` 或 Linux 的 `/dev/ttyACMx`。

## 预期现象

显示第一张图片，可通过左右滑动切换图片。

## 常见问题

启动前，将 `data/` 的内容复制到 FAT32 microSD 卡的 `/images/` 目录，并确保其中至少有一张受支持的图片。`.bin` 必须是与本例程颜色配置匹配的 LVGL 8 图片数据，不能使用固件 BIN。
