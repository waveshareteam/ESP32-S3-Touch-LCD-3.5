# 09_gfx_jpeg — JPEG 解码

[English](README.md) | [简体中文](README_ZH.md)

## 功能

使用 JPEGDEC 解码 microSD 卡上的 `/images/octocat.jpg`，并通过 Arduino_GFX 显示。

## 硬件需求

- ESP32-S3-Touch-LCD-3.5 不带外壳版，或 ESP32-S3-Touch-LCD-3.5-C 带外壳版。
- USB 数据线；按下方说明准备摄像头、扬声器或 microSD 卡等所需外设。

## 怎么运行

1. 根据[开发环境](../../../../README_ZH.md#development-environment)安装 Arduino-ESP32，并将[随附库](../../libraries/)加入草图簿的 `libraries` 目录。
2. 选择 **ESP32S3 Dev Module**，Flash **16MB**、PSRAM **OPI PSRAM**，其余选项按首页配置。
3. 打开 `09_gfx_jpeg.ino` 并上传；串口监视器波特率设为 **115200**。

## 预期现象

LCD 显示 JPEG 图片。

## 常见问题

在 SD 卡根目录新建 `images` 文件夹，将 `data/` 中的内容复制进去，最终路径为 `/images/octocat.jpg`。替换图片时使用基线 JPEG。
