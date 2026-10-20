# 02_lvgl_example — LVGL 控件与触摸

[English](README.md) | [简体中文](README_ZH.md)

## 功能

在 320 × 480 屏幕上运行 LVGL 8 控件演示。

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

屏幕显示控件演示，可通过触摸操作。

## 常见问题

默认运行 `main/main.cpp` 中的 `lv_demo_widgets()`；切换演示前需启用对应的 LVGL 配置。
