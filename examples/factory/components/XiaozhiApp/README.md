# AIChats component

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Integrates Xiaozhi activation, wake-word detection, speech conversation and camera analysis with the Brookesia application lifecycle. The [factory example](../../README.md) installs this component as AIChats.

## Hardware Required

Uses the board's ES8311 codec and one microphone, without a hardware echo-reference channel. Camera analysis shares the BSP camera driver and lock with the Camera application.

## How to Run

Build and flash the complete factory project, including the speech-model partition. Connect Wi-Fi in Settings, open AIChats, complete activation and say “你好小智” or tap Talk; tap Stop to cancel.

## Expected Result

Recognition and replies appear as subtitles and audio. When the server invokes the camera tool, the application shows a local preview and the captured image during analysis; BOOT queues a 180-degree rotation for the next photo, and leaving the camera view restores the conversation layout.

## Troubleshooting

Camera analysis requires vision-service configuration supplied by the server; capture, upload and service errors are reported separately in the serial log. Image/audio buffers use PSRAM, while the MCP worker stack stays in internal RAM because NVS access can disable the external-memory cache; upstream OGG adaptation attribution and its MIT license are retained in [third_party/xiaozhi_esp32](third_party/xiaozhi_esp32/).
