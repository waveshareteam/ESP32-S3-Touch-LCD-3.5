# 03_lvgl_system — 系统信息、RTC 与音频控制

[English](README.md) | [简体中文](README_ZH.md)

## 功能

显示包含 RTC、存储信息以及音频和显示控制的系统页面。

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

系统页面显示外设信息，控件通过已初始化的音频和显示接口工作。

## 常见问题

查看存储信息需插入 FAT32 microSD 卡；音频输出需连接扬声器。RTC 初始化使用示例中的日期设置，不是联网校时。
