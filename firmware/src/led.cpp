#include "led.h"

static LedPattern s_pattern = LED_OFF;
static LedPattern s_base = LED_STEADY;
static uint32_t s_one_shot_until = 0;
static bool s_level = false;

static void led_write(bool on) {
#if LED_ACTIVE_LOW
    digitalWrite(LED_PIN, on ? LOW : HIGH);
#else
    digitalWrite(LED_PIN, on ? HIGH : LOW);
#endif
}

void led_begin() {
    pinMode(LED_PIN, OUTPUT);
    led_write(false);
}

void led_set(LedPattern p) {
    s_base = p;
    if (s_one_shot_until == 0) s_pattern = p;
}

void led_flash(LedPattern one_shot) {
    s_pattern = one_shot;
    s_one_shot_until = millis() + 220;
}

void led_tick() {
    if (s_one_shot_until != 0 && millis() > s_one_shot_until) {
        s_one_shot_until = 0;
        s_pattern = s_base;
    }

    uint32_t t = millis();
    switch (s_pattern) {
        case LED_OFF:
            s_level = false;
            break;
        case LED_STEADY:
            s_level = true;
            break;
        case LED_HEARTBEAT: {
            uint32_t m = t % 2000;
            s_level = m < 60 || (m >= 160 && m < 220);
            break;
        }
        case LED_BLINK_SLOW:
            s_level = (t % 1000) < 500;
            break;
        case LED_BLINK_FAST:
            s_level = (t % 150) < 75;
            break;
        case LED_FLASH2: {
            uint32_t m = t % 220;
            s_level = m < 50 || (m >= 90 && m < 140);
            break;
        }
    }
    led_write(s_level);
}
