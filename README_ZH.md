<div align="center">
  <h1>ESP32-S3-Touch-LCD-3.5</h1>
  <p><strong>ESP32-S3 3.5 英寸 320 × 480 SPI IPS 电容触摸屏开发板</strong></p>
  <p><a href="README.md">English</a> | <strong>简体中文</strong></p>
  <p>
    <a href="https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/actions/workflows/build.yml"><img alt="Build" src="https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/actions/workflows/build.yml/badge.svg"></a>
    <a href="https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/releases/latest"><img alt="Latest Release" src="https://img.shields.io/github/v/release/waveshareteam/ESP32-S3-Touch-LCD-3.5"></a>
    <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/waveshareteam/ESP32-S3-Touch-LCD-3.5"></a>
  </p>
  <p>
    <a href="https://www.waveshare.net/shop/ESP32-S3-Touch-LCD-3.5.htm">🌐 产品</a> &middot;
    <a href="firmware/">📦 固件</a> &middot;
    <a href="examples/esp-idf/">🧩 ESP-IDF</a> &middot;
    <a href="examples/arduino/examples/">🔧 Arduino</a> &middot;
    <a href="hardware/schematic/">📚 资料</a>
  </p>
  <p><img src="docs/images/esp32-s3-touch-lcd-3.5-c-1.jpg" alt="ESP32-S3-Touch-LCD-3.5-C 带外壳版，包含 OV5640 摄像头" width="500"></p>
  <p><strong>ESP32-S3-Touch-LCD-3.5-C · 带外壳版</strong></p>
  <p><img src="docs/images/esp32-s3-touch-lcd-3.5-board.png" alt="ESP32-S3-Touch-LCD-3.5 不带外壳版背面" width="500"></p>
  <p><strong>ESP32-S3-Touch-LCD-3.5 · 不带外壳版</strong></p>
</div>

---

## ✨ 概述

ESP32-S3-Touch-LCD-3.5 将 ESP32-S3、3.5 英寸 IPS 电容触摸屏、电源管理、六轴传感器、RTC 与音频编解码器集成在同一开发板上。本仓库提供 Arduino 与 ESP-IDF 示例、出厂固件及原理图。

本产品有**带外壳**和**不带外壳**两个版本，均使用本仓库的代码。

| 产品版本 | 外壳 | 摄像头 | 产品页 |
| --- | --- | --- | --- |
| ESP32-S3-Touch-LCD-3.5 | 不带外壳 | 板载摄像头接口，模块需另配 | [不带外壳版](https://www.waveshare.net/shop/ESP32-S3-Touch-LCD-3.5.htm) |
| ESP32-S3-Touch-LCD-3.5-C | 带外壳 | 包含 OV5640 摄像头 | [带外壳版](https://www.waveshare.net/shop/ESP32-S3-Touch-LCD-3.5-C.htm) |

外壳款式与 PCB 的 V1/V2 硬件修订版是两个不同概念。请根据 PCB 标注选择对应原理图。

| 特性 | 器件 / 接口 |
| --- | --- |
| 主控 | ESP32-S3，双核，最高 240 MHz |
| 存储 | 16 MB Flash、8 MB PSRAM |
| 无线 | 2.4 GHz Wi-Fi、Bluetooth LE |
| 显示 | 3.5 英寸 320 × 480 IPS LCD，ST7796，SPI |
| 触摸 | FT6336 电容触摸，I2C |
| 电源管理 | AXP2101 |
| 运动传感器 | QMI8658 六轴 IMU |
| 时钟 | PCF85063 RTC |
| 音频 | ES8311 编解码器、麦克风、扬声器接口 |
| 存储扩展 | microSD，SD_MMC 1-bit |
| 扩展与摄像头 | TCA9554 I/O 扩展器、兼容 OV5640 的摄像头接口 |

## 📦 快速开始

1. 从 [firmware](firmware/) 下载出厂合并固件；使用示例固件时，从 [Releases](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/releases) 选择已有的 `*-combined.zip` 固件包。
2. 用 USB 数据线连接开发板，安装烧录工具：`python -m pip install esptool`。
3. 出厂固件按 [烧录说明](firmware/README_ZH.md) 从 `0x0` 烧录；示例固件包解压后运行 `flash_combined.bat COMx` 或 `./flash_combined.sh /dev/ttyACM0`。
4. 复位开发板，按所选示例的 README 准备 SD 卡、扬声器或摄像头。
5. 需要修改程序时，选择下方示例，在对应目录编译和烧录。

出厂固件位于 [firmware/](firmware/)，保留新版应用桌面与旧版综合演示。新版源码位于 [examples/factory/](examples/factory/README_ZH.md)，旧版源码位于 [01_factory](examples/esp-idf/01_factory/README_ZH.md)。

## 🧪 示例

### ESP-IDF

| 示例 | 主要功能 |
| --- | --- |
| [01_factory](examples/esp-idf/01_factory/README_ZH.md) | 旧版 LVGL 8 出厂综合演示 |
| [02_lvgl_example](examples/esp-idf/02_lvgl_example/README_ZH.md) | LVGL 控件与触摸 |
| [03_lvgl_system](examples/esp-idf/03_lvgl_system/README_ZH.md) | 系统信息、RTC 与音频控制 |
| [04_lvgl_qmi8658](examples/esp-idf/04_lvgl_qmi8658/README_ZH.md) | QMI8658 运动数据显示 |
| [05_lvgl_axp2101](examples/esp-idf/05_lvgl_axp2101/README_ZH.md) | AXP2101 电源信息 |
| [06_lvgl_camera](examples/esp-idf/06_lvgl_camera/README_ZH.md) | 摄像头预览 |
| [07_lvgl_wifi](examples/esp-idf/07_lvgl_wifi/README_ZH.md) | Wi-Fi 连接与扫描界面 |
| [08_lvgl_image](examples/esp-idf/08_lvgl_image/README_ZH.md) | SD 卡图片浏览 |

### Arduino

| 示例 | 主要功能 |
| --- | --- |
| [01_audio_out](examples/arduino/examples/01_audio_out/README_ZH.md) | SD 卡 MP3 播放 |
| [02_axp2101_example](examples/arduino/examples/02_axp2101_example/README_ZH.md) | 电源管理测试 |
| [03_camera_web_server](examples/arduino/examples/03_camera_web_server/README_ZH.md) | 摄像头网页服务器 |
| [04_es8311_example](examples/arduino/examples/04_es8311_example/README_ZH.md) | 麦克风录音与回放 |
| [05_pcf85063_time](examples/arduino/examples/05_pcf85063_time/README_ZH.md) | RTC 串口输出 |
| [06_qmi8658_getdata](examples/arduino/examples/06_qmi8658_getdata/README_ZH.md) | 运动传感器串口输出 |
| [07_sd_card_test](examples/arduino/examples/07_sd_card_test/README_ZH.md) | microSD 文件操作 |
| [08_gfx_helloworld](examples/arduino/examples/08_gfx_helloworld/README_ZH.md) | GFX 显示测试 |
| [09_gfx_jpeg](examples/arduino/examples/09_gfx_jpeg/README_ZH.md) | JPEG 解码 |
| [10_gfx_png](examples/arduino/examples/10_gfx_png/README_ZH.md) | PNG 解码 |
| [11_lvgl_arduino_v8](examples/arduino/examples/11_lvgl_arduino_v8/README_ZH.md) | LVGL 8 触摸控件 |
| [12_lvgl_pcf85063](examples/arduino/examples/12_lvgl_pcf85063/README_ZH.md) | RTC 界面 |
| [13_lvgl_qmi8658](examples/arduino/examples/13_lvgl_qmi8658/README_ZH.md) | 运动传感器界面 |
| [14_lvgl_image](examples/arduino/examples/14_lvgl_image/README_ZH.md) | LVGL SD 图片显示 |

Arduino 所需库集中放在 [examples/arduino/libraries/](examples/arduino/libraries/)。使用随附库，保留各 LVGL 草图自己的 `lv_conf.h`；图片例程的解码器和颜色配置与普通控件例程不同。

### 出厂演示

[factory](examples/factory/README_ZH.md) 使用 LVGL 9 应用桌面，包含 Settings、AIChats、Touch Test、Photos、Weather、Music、Sensor、2048、Camera 和 Mic Test。Settings 与 AIChats 排在前两位，板级初始化集中由 BSP 完成。

<a id="development-environment"></a>

## 🛠️ 开发环境

| 框架 | 配置版本 | 示例数 |
| --- | --- | ---: |
| ESP-IDF | `v5.5.4` | 8 |
| ESP-IDF（出厂工程） | `v5.5.4` | 1 |
| Arduino-ESP32 | `3.3.11` | 14 |

框架版本及完整 FQBN 以 [.github/product.json](.github/product.json) 为准。单项外设示例及旧版综合演示使用 **LVGL 8.4**，新版出厂桌面独立使用 **LVGL 9.6** 依赖。

Arduino IDE 选择 **ESP32S3 Dev Module**：Flash **16MB**、PSRAM **OPI PSRAM**、Flash Mode **QIO**、Partition Scheme **16M Flash (3MB APP/9.9MB FATFS)**、USB Mode **Hardware CDC and JTAG**、USB CDC On Boot **Enabled**。将随附库放入 Arduino 草图簿的 `libraries` 目录，然后打开对应 `.ino` 文件。

ESP-IDF 用户在已启用的工具环境中进入所选示例目录，执行 `idf.py set-target esp32s3`、`idf.py build`、`idf.py -p PORT flash monitor`。首次构建会下载清单中的托管组件。

## 📚 资料

- [V1 原理图](hardware/schematic/ESP32-S3-Touch-LCD-3.5-V1-Schematic.pdf)
- [V2 原理图](hardware/schematic/ESP32-S3-Touch-LCD-3.5-V2-Schematic.pdf)
- [不带外壳版产品页](https://www.waveshare.net/shop/ESP32-S3-Touch-LCD-3.5.htm)
- [带外壳版产品页](https://www.waveshare.net/shop/ESP32-S3-Touch-LCD-3.5-C.htm)
- [中文 Wiki](https://docs.waveshare.net/ESP32-S3-Touch-LCD-3.5/)

## 🤝 支持

- 产品使用及硬件问题：[微雪技术支持](https://docs.waveshare.net/ESP32-S3-Touch-LCD-3.5/Technical-Support/)。

## 📄 许可证

[Apache License 2.0](LICENSE)；第三方组件保留各自的许可证与版权声明。
