#pragma once
#include "systems/phone/widgets/status_bar/esp_brookesia_status_bar.hpp"
// Feed network time and S3 BSP battery readings into the standard status bar.
bool desktop_status_start(esp_brookesia::systems::phone::StatusBar *bar);
