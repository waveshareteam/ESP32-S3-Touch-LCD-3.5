# 01_factory — 旧版出厂综合演示

[English](README.md) | [简体中文](README_ZH.md)

## 功能

初始化显示、触摸、电源管理、音频、IMU、RTC、SD 卡、摄像头及 Wi-Fi，运行原有 LVGL 8 综合演示。新版应用桌面独立维护在 [factory](../../factory/README_ZH.md)。

## 硬件需求

使用 ESP32-S3-Touch-LCD-3.5 或 ESP32-S3-Touch-LCD-3.5-C，准备 USB 数据线。测试对应外设时，连接兼容摄像头和扬声器，并插入 FAT32 microSD 卡。

## 怎么运行

启用[开发环境](../../../README_ZH.md#development-environment)中的 ESP-IDF 环境，在本目录执行 `idf.py set-target esp32s3`、`idf.py build`、`idf.py -p PORT flash monitor`。将 `PORT` 替换为实际串口。

## 预期现象

启动后先进入触摸测试。按一次 BOOT 结束测试，进入旧版综合演示界面。

## 常见问题

Wi-Fi 账号在 `main/main.cpp` 中配置，可选外设的初始化失败会打印在串口日志中。本示例独立保留原有 LVGL 8 依赖，不与新版出厂桌面共用配置。
