# 04_es8311_example — 麦克风录音与回放

[English](README.md) | [简体中文](README_ZH.md)

## 功能

将五秒麦克风音频录制到 RAM，然后通过 I2S 和 ES8311 播放 WAV。

## 硬件需求

- ESP32-S3-Touch-LCD-3.5 不带外壳版，或 ESP32-S3-Touch-LCD-3.5-C 带外壳版。
- USB 数据线；按下方说明准备摄像头、扬声器或 microSD 卡等所需外设。

## 怎么运行

1. 根据[开发环境](../../../../README_ZH.md#development-environment)安装 Arduino-ESP32，并将[随附库](../../libraries/)加入草图簿的 `libraries` 目录。
2. 选择 **ESP32S3 Dev Module**，Flash **16MB**、PSRAM **OPI PSRAM**，其余选项按首页配置。
3. 打开 `04_es8311_example.ino` 并上传；串口监视器波特率设为 **115200**。

## 预期现象

复位后对麦克风说话，随后扬声器回放录音。

## 常见问题

需要连接扬声器。本示例将录音保存在 RAM，不会将 WAV 保存到 SD 卡。
