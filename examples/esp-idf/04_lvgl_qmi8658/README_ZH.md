# 04_lvgl_qmi8658 — QMI8658 运动数据显示

[English](README.md) | [简体中文](README_ZH.md)

## 功能

读取板载 QMI8658 加速度计和陀螺仪，并在 LVGL 中显示测量值。

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

移动或旋转开发板时，界面中的运动数据随之变化。

## 常见问题

检查静止读数时保持开发板不动；初始化失败时检查 I2C 日志，并核对对应硬件修订版的原理图。
