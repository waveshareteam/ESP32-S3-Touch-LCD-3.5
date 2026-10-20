/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_err.h"

struct esp_mcp_s;
typedef struct esp_mcp_s esp_mcp_t;

namespace esp_brookesia::apps {

/**
 * Register product-specific MCP tools exposed by the Xiaozhi application.
 *
 * This hook runs once for each newly created MCP engine, before the Xiaozhi
 * transport starts. Registers the camera capture/explain tool and observes
 * the server's vision capability on the dedicated MCP worker.
 */
esp_err_t registerXiaozhiDeveloperTools(esp_mcp_t *mcp);
void setXiaozhiCameraEnabled(bool enabled);
// Poll only from the foreground APP. This queues a change; I2C runs on MCP.
void pollXiaozhiCameraButton();

} // namespace esp_brookesia::apps
