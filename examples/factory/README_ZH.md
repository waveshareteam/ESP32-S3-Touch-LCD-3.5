# 出厂应用工程

[English](README.md) | [简体中文](README_ZH.md)

## 功能

提供 320 × 480 竖屏桌面，包含 Settings、AIChats、Touch Test、Photos、Weather、Music、Sensor、2048、Camera、Mic Test、Button Test 和 Display Test，共 12 个应用。

## 硬件需求

使用 ESP32-S3-Touch-LCD-3.5 或 ESP32-S3-Touch-LCD-3.5-C，准备 USB 数据线。摄像头、音频和 SD 功能分别需要兼容摄像头、扬声器和 FAT32 microSD 卡；带外壳版包含 OV5640 摄像头。

## 怎么运行

恢复出厂软件请参照[固件烧录说明](../../firmware/README_ZH.md)。从源码编译时，请按[开发环境](../../README_ZH.md#development-environment)配置 ESP-IDF v5.5.4，在本目录依次执行 `idf.py set-target esp32s3`、`idf.py build`、`idf.py -p PORT flash monitor`，将 `PORT` 替换为设备的实际串口。

## 预期现象

启动后显示 12 个应用，Settings 位于第一、AIChats 位于第二，点击图标进入应用。启动完成后，屏幕亮起并显示桌面。设置中可调背光和音量、扫描或配置 Wi-Fi、查看 SD 信息；JPEG 图片和 MP3 音频分别放入 SD 卡根目录的 `/photo`、`/music` 文件夹。

短按 PWR 按键一次开机，长按 5 秒关机。在 Button Test 中测试 PWR 时请短按。

各应用操作与配网步骤请参照[使用说明](USER_GUIDE_ZH.md)。

## 常见问题

天气和 AIChats 需要能够访问互联网，AIChats 需按屏幕提示完成激活。若提示缺少语音模型，请烧录完整出厂固件或执行完整的 `idf.py flash`，不要只烧录应用；其他问题请参照[使用说明](USER_GUIDE_ZH.md#常见问题)。
