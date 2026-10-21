# Factory firmware user guide

[English](USER_GUIDE.md) | [简体中文](USER_GUIDE_ZH.md)

## Getting started

Use a USB data cable to power ESP32-S3-Touch-LCD-3.5 or the enclosed ESP32-S3-Touch-LCD-3.5-C. To restore the factory software, follow the [firmware instructions](../../firmware/README.md); to build from source, follow the [factory README](README.md).

The desktop is portrait (320 × 480) and has three pages. Swipe left or right to switch pages; Settings is first and AIChats is second. Most applications have a back arrow; in Photos, swipe upward from the bottom edge to return. Touch Test also supports BOOT to exit.

Short press PWR once to power on; press and hold PWR for 5 seconds to power off. During startup, the backlight remains off until the first desktop frame has been transferred.

## Connecting to Wi-Fi

The board supports 2.4 GHz Wi-Fi. Saved credentials take priority; otherwise it tries the demonstration hotspot `weather-wifi`, password `12345678`.

1. To use another network, open **Settings → Wi-Fi setup**.
2. Scan the displayed QR code, or join the hotspot whose name is shown on screen. Its password is `12345678`.
3. Open `http://192.168.4.1` while connected to that hotspot, enter your router's SSID and password, and submit.
4. Wait for the connection result on the page and device. Credentials are saved after a successful connection.

**Wi-Fi scan** only lists nearby networks, signal strength, security and channel. It does not connect to a selected network. An empty SSID is shown as hidden; this is different from a Chinese network name.

After an Internet connection is available, network time updates the desktop clock in UTC+8. The Sensor application's hardware RTC remains independent.

## Preparing an SD card

Use a FAT32 microSD card. Create these folders at the card root before inserting it:

```text
/photo/       JPEG photos (.jpg or .jpeg)
/music/       MP3 music (.mp3)
```

Use 320 × 480 JPEGs for predictable memory usage and quick loading. Photos scans and decodes files when the application opens. Exit media applications before removing the card; the firmware prevents unmounting while an application holds a card lease.

## Applications

| Application | Operation |
| --- | --- |
| Settings | Adjust screen brightness and audio volume; scan Wi-Fi, configure a network, check SD-card information and view About. |
| AIChats | Connect to the Internet and complete activation using the code and instructions on screen. Say “你好小智” or tap **Talk** to start a conversation. Ask about the camera view to invoke a photo and vision analysis; the captured view appears on screen. |
| Touch Test | Draw across the screen to check touch coverage. Use the upper-left back button or BOOT to return. |
| Photos | View JPEGs from `/photo`; swipe left/right to change images and upward from the bottom edge to exit. |
| Weather | Fetch weather over the Internet using the included service configuration. The account/location can be changed in the weather menu under `idf.py menuconfig`. |
| Music | Play MP3s from `/music`; use previous, next and the combined play/pause button. Adjust volume in Settings. |
| Sensor | View hardware RTC time and QMI8658 accelerometer/gyroscope data. |
| 2048 | Swipe to move tiles; tap **New** to start again. |
| Camera | View the camera, press BOOT to rotate 180°, and tap **Capture** to save a JPEG in `/photo`. The saved picture can be opened in Photos. |
| Mic Test | Watch the microphone level without an SD card. Insert a card, tap **Record** to save up to 10 seconds, then **Play** to replay the latest recording. Files are saved under `/recordings`. |
| Button Test | Press and release BOOT and PWR to check their states, counts and PASS results. Tap **Test again** to reset. BOOT is active-low on GPIO0; PWR SYS_OUT is active-high on TCA9554 EXIO6. |
| Display Test | Tap the five-color monitor icon to cycle full-screen red, green, blue, yellow and white at one-second intervals. Double tap anywhere on the screen during any color to return to the desktop. |

Button Test samples the keys while open and counts complete press/release cycles. If a key is held when entering the page or resetting, release it before pressing again. PWR retains its long-hold power-off function, so use short presses. Exit using the on-screen back arrow.

Music, Mic Test and AIChats share one audio device exclusively. Exit the current audio application before opening another one.

## Common questions

- **Wi-Fi connects but weather or AIChats does not work:** check that the hotspot has Internet access. Network connection alone does not guarantee that the remote weather or voice service is reachable.
- **The desktop time is unavailable:** wait for network time synchronization; check Internet access and UDP/NTP availability. This does not change the Sensor RTC.
- **Photos or Music has no files:** check the folder names, supported extensions and card format. Reopen the application after adding files.
- **AIChats reports a missing speech model:** flash the complete project or factory merged image, including the speech-model partition. An application-only BIN does not install it.
- **Audio is busy:** close the current Music/Mic Test/AIChats session, then reopen the desired application.
- **Camera preview works but AI analysis fails:** check the network and vision-service result; local capture and remote analysis are separate steps.
