#pragma once

#include <Arduino.h>

#define LOGI(tag, fmt, ...) Serial.printf("[I][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define LOGW(tag, fmt, ...) Serial.printf("[W][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define LOGE(tag, fmt, ...) Serial.printf("[E][%s] " fmt "\n", tag, ##__VA_ARGS__)
