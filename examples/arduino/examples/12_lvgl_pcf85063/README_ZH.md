# 12_lvgl_pcf85063 — RTC 界面

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 LVGL 界面显示 PCF85063 的日期和时间。

## 硬件需求

- ESP32-S3-Touch-LCD-3.5 不带外壳版，或 ESP32-S3-Touch-LCD-3.5-C 带外壳版。
- USB 数据线；按下方说明准备摄像头、扬声器或 microSD 卡等所需外设。

## 怎么运行

1. 根据[开发环境](../../../../README_ZH.md#development-environment)安装 Arduino-ESP32，并将[随附库](../../libraries/)加入草图簿的 `libraries` 目录。
2. 选择 **ESP32S3 Dev Module**，Flash **16MB**、PSRAM **OPI PSRAM**，其余选项按首页配置。
3. 打开 `12_lvgl_pcf85063.ino` 并上传；串口监视器波特率设为 **115200**。

## 预期现象

LCD 显示 RTC 日期和时间。

## 常见问题

草图会初始化示例日期；保留本目录的 `lv_conf.h`，按需要修改日期初始化值。
