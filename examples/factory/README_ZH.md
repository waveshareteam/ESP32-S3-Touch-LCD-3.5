# factory — 应用桌面

[English](README.md) | [简体中文](README_ZH.md)

## 功能

运行 320 × 480 竖屏桌面，包含 Settings、AIChats、Touch Test、Photos、Weather、Music、Sensor、2048、Camera 和 Mic Test。外设由 BSP 初始化，应用使用 LVGL 9.6 与 Brookesia 生命周期封装。

## 硬件需求

使用 ESP32-S3-Touch-LCD-3.5 或 ESP32-S3-Touch-LCD-3.5-C，准备 USB 数据线。摄像头、音频和 SD 功能分别需要兼容摄像头、扬声器和 FAT32 microSD 卡；带外壳版包含 OV5640 摄像头。

## 怎么运行

使用[开发环境](../../README_ZH.md#development-environment)中列出的出厂工程 ESP-IDF 版本，依次执行 `idf.py set-target esp32s3`、`idf.py build`、`idf.py -p PORT flash monitor`。工程内置原有天气 API 配置，可按需在 `idf.py menuconfig` → **S3 3.5 Weather** 修改账号及地区。

## 预期现象

启动后显示桌面，Settings 位于第一、AIChats 位于第二，点击图标进入应用。设置中可调背光和音量、扫描或配置 Wi-Fi、查看 SD 信息；JPEG 图片和 MP3 音频分别放入 SD 卡根目录的 `/photo`、`/music` 文件夹。

各应用操作与配网步骤请参照[使用说明](USER_GUIDE_ZH.md)。

## 常见问题

联网优先使用已保存的账号，否则使用 [wifi_manager.h](components/network_service/wifi_manager.h) 中的演示网络；其他网络通过 **Settings → Wi-Fi setup** 配置，扫描屏幕二维码或连接其热点后访问 `http://192.168.4.1`。天气使用内置的 `cn.apihz.cn` API 配置；AIChats 需要联网激活，首次烧录须使用完整的 `idf.py flash`，包含 `0x810000` 处的语音模型，不能仅烧录应用。
