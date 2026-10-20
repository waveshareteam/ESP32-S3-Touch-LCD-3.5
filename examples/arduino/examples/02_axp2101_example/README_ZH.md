# 02_axp2101_example — 电源管理测试

[English](README.md) | [简体中文](README_ZH.md)

## 功能

初始化 AXP2101，并向串口输出电源和电池信息。

## 硬件需求

- ESP32-S3-Touch-LCD-3.5 不带外壳版，或 ESP32-S3-Touch-LCD-3.5-C 带外壳版。
- USB 数据线；按下方说明准备摄像头、扬声器或 microSD 卡等所需外设。

## 怎么运行

1. 根据[开发环境](../../../../README_ZH.md#development-environment)安装 Arduino-ESP32，并将[随附库](../../libraries/)加入草图簿的 `libraries` 目录。
2. 选择 **ESP32S3 Dev Module**，Flash **16MB**、PSRAM **OPI PSRAM**，其余选项按首页配置。
3. 打开 `02_axp2101_example.ino` 并上传；串口监视器波特率设为 **115200**。

## 预期现象

串口监视器输出电源管理芯片信息。

## 常见问题

读取电池信息需连接电池；修改电源输出电压前请核对开发板原理图。
