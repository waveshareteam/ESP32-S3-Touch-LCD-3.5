# 05_lvgl_axp2101 — AXP2101 电源信息

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 LVGL 页面显示 AXP2101 电源管理芯片的测量信息。

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

屏幕显示当前可读取的电源与电池信息。

## 常见问题

电池读数需要连接兼容电池；调整电源输出电压前应核对原理图。
