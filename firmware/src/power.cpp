#include "power.h"

#include <driver/gpio.h>
#include <esp_sleep.h>

#include "button.h"
#include "config.h"
#include "led.h"
#include "log.h"
#include "settings.h"

static const char* TAG = "power";

static uint32_t s_last_activity = 0;

void power_note_activity() { s_last_activity = millis(); }

void power_sleep_now() {
    LOGI(TAG, "deep sleep — press shutter to wake");
    led_set(LED_OFF);
    led_tick();
    // Button low (pressed) wakes the chip. Internal pull may not be retained
    // in deep sleep on the S3 — the hardware guide specifies an external
    // 100k pull-up on the shutter line.
    gpio_pullup_en((gpio_num_t)BTN_PIN_SHUTTER);
    gpio_pulldown_dis((gpio_num_t)BTN_PIN_SHUTTER);
    gpio_wakeup_enable((gpio_num_t)BTN_PIN_SHUTTER, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    esp_deep_sleep_start();
}

void power_tick() {
    if (g_settings.sleep_min == 0 || s_last_activity == 0) return;
    uint32_t idle = millis() - s_last_activity;
    if (idle > (uint32_t)g_settings.sleep_min * 60u * 1000u) {
        power_sleep_now();
    }
}

bool power_woken_by_button() {
    return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO;
}
