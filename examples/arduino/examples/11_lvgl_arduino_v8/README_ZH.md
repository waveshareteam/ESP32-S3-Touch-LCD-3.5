# 11_lvgl_arduino_v8 — LVGL 8 触摸控件

[English](README.md) | [简体中文](README_ZH.md)

## 功能

使用 LVGL 8 显示问候文字和两个触摸开关。

## 硬件需求

- ESP32-S3-Touch-LCD-3.5 不带外壳版，或 ESP32-S3-Touch-LCD-3.5-C 带外壳版。
- USB 数据线；按下方说明准备摄像头、扬声器或 microSD 卡等所需外设。

## 怎么运行

1. 根据[开发环境](../../../../README_ZH.md#development-environment)安装 Arduino-ESP32，并将[随附库](../../libraries/)加入草图簿的 `libraries` 目录。
2. 选择 **ESP32S3 Dev Module**，Flash **16MB**、PSRAM **OPI PSRAM**，其余选项按首页配置。
3. 打开 `11_lvgl_arduino_v8.ino` 并上传；串口监视器波特率设为 **115200**。

## 预期现象

屏幕显示 “Hello Arduino!” 文字，开关可通过触摸操作。

## 常见问题

使用随附 LVGL 8.4 库和本草图的 `lv_conf.h`，不要直接替换为 LVGL 9。
