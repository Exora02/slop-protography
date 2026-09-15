#pragma once

#include <Arduino.h>

#include "config.h"

// Status LED patterns — the device's only display. Non-blocking; call
// led_tick() from the main loop.
//
// steady      ready
// heartbeat   ready + at least one phone connected (soft double-pulse)
// blink_slow  live view streaming to a phone
// blink_fast  error (camera init failure)
// flash_2     photo captured (one-shot pattern)
// off         sleeping / shutting down

enum LedPattern : uint8_t {
    LED_OFF = 0,
    LED_STEADY,
    LED_HEARTBEAT,
    LED_BLINK_SLOW,
    LED_BLINK_FAST,
    LED_FLASH2,
};

void led_begin();
void led_set(LedPattern p);
void led_flash(LedPattern one_shot);   // show a pattern once, then return
void led_tick();
void led_suspend();                    // release the pin (SD owns GPIO21 on
                                       // the Sense expansion board); the LED
                                       // then flickers with card traffic
