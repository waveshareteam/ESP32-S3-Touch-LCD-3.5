# 03_camera_web_server — 摄像头网页服务器

[English](README.md) | [简体中文](README_ZH.md)

## 功能

连接 Wi-Fi，并通过浏览器页面提供摄像头画面。

## 硬件需求

- ESP32-S3-Touch-LCD-3.5 不带外壳版，或 ESP32-S3-Touch-LCD-3.5-C 带外壳版。
- USB 数据线；按下方说明准备摄像头、扬声器或 microSD 卡等所需外设。

## 怎么运行

1. 根据[开发环境](../../../../README_ZH.md#development-environment)安装 Arduino-ESP32，并将[随附库](../../libraries/)加入草图簿的 `libraries` 目录。
2. 选择 **ESP32S3 Dev Module**，Flash **16MB**、PSRAM **OPI PSRAM**，其余选项按首页配置。
3. 打开 `03_camera_web_server.ino` 并上传；串口监视器波特率设为 **115200**。

## 预期现象

串口输出摄像头访问地址，同一网络内的浏览器可打开该页面。

## 常见问题

烧录前修改草图中的 `ssid` 和 `password`。需要兼容摄像头模块，-C 版本已包含 OV5640。
