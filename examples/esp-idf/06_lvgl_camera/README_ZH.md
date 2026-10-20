# 06_lvgl_camera — 摄像头预览

[English](README.md) | [简体中文](README_ZH.md)

## 功能

使用随附摄像头驱动和 LVGL 界面，在 LCD 上显示摄像头画面。

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

摄像头页面显示已连接摄像头的预览画面。

## 常见问题

需要摄像头模块。-C 带外壳版包含 OV5640；不带外壳版需另配兼容模块。无画面时检查排线和初始化日志。
