# 06_qmi8658_getdata — 运动传感器串口输出

[English](README.md) | [简体中文](README_ZH.md)

## 功能

读取 QMI8658 的加速度与角速度。

## 硬件需求

- ESP32-S3-Touch-LCD-3.5 不带外壳版，或 ESP32-S3-Touch-LCD-3.5-C 带外壳版。
- USB 数据线；按下方说明准备摄像头、扬声器或 microSD 卡等所需外设。

## 怎么运行

1. 根据[开发环境](../../../../README_ZH.md#development-environment)安装 Arduino-ESP32，并将[随附库](../../libraries/)加入草图簿的 `libraries` 目录。
2. 选择 **ESP32S3 Dev Module**，Flash **16MB**、PSRAM **OPI PSRAM**，其余选项按首页配置。
3. 打开 `06_qmi8658_getdata.ino` 并上传；串口监视器波特率设为 **115200**。

## 预期现象

移动或旋转开发板时，串口数据随之变化。

## 常见问题

初始化失败时检查 I2C 连接及对应硬件修订版的原理图。
