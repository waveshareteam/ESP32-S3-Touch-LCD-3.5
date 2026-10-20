# 01_audio_out — SD 卡 MP3 播放

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 ES8311 和扬声器播放 microSD 卡根目录的 `/1.mp3`。

## 硬件需求

- ESP32-S3-Touch-LCD-3.5 不带外壳版，或 ESP32-S3-Touch-LCD-3.5-C 带外壳版。
- USB 数据线；按下方说明准备摄像头、扬声器或 microSD 卡等所需外设。

## 怎么运行

1. 根据[开发环境](../../../../README_ZH.md#development-environment)安装 Arduino-ESP32，并将[随附库](../../libraries/)加入草图簿的 `libraries` 目录。
2. 选择 **ESP32S3 Dev Module**，Flash **16MB**、PSRAM **OPI PSRAM**，其余选项按首页配置。
3. 打开 `01_audio_out.ino` 并上传；串口监视器波特率设为 **115200**。

## 预期现象

连接的扬声器播放音频。

## 常见问题

启动前将 `data/1.mp3` 复制到 FAT32 microSD 卡根目录，并启用 OPI PSRAM。
