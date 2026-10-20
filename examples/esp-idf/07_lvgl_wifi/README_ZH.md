# 07_lvgl_wifi — Wi-Fi 连接与扫描界面

[English](README.md) | [简体中文](README_ZH.md)

## 功能

初始化 Wi-Fi 并显示 LVGL Wi-Fi 页面。

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

Wi-Fi 页面显示扫描结果和连接状态。

## 常见问题

将 `main/main.cpp` 中传给 `esp_wifi_port_init()` 的 SSID 和密码改为自己的 2.4 GHz 网络；随附值仅用于演示。
