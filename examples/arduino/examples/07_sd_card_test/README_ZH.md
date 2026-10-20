# 07_sd_card_test — microSD 文件操作

[English](README.md) | [简体中文](README_ZH.md)

## 功能

测试 SD_MMC 挂载、目录操作、文件读写和容量读取。

## 硬件需求

- ESP32-S3-Touch-LCD-3.5 不带外壳版，或 ESP32-S3-Touch-LCD-3.5-C 带外壳版。
- USB 数据线；按下方说明准备摄像头、扬声器或 microSD 卡等所需外设。

## 怎么运行

1. 根据[开发环境](../../../../README_ZH.md#development-environment)安装 Arduino-ESP32，并将[随附库](../../libraries/)加入草图簿的 `libraries` 目录。
2. 选择 **ESP32S3 Dev Module**，Flash **16MB**、PSRAM **OPI PSRAM**，其余选项按首页配置。
3. 打开 `07_sd_card_test.ino` 并上传；串口监视器波特率设为 **115200**。

## 预期现象

串口输出卡类型、容量及文件操作结果。

## 常见问题

使用 FAT32 测试卡。草图会创建、覆盖或删除 `/mydir`、`/hello.txt`、`/foo.txt` 和 `/test.txt` 等测试路径，测试卡上不要放置同名的重要文件。
