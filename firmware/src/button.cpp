#include "button.h"

Button::Button(uint8_t pin) : pin_(pin) {}

void Button::begin() {
    pinMode(pin_, INPUT_PULLUP);
}

BtnEvent Button::tick() {
    uint32_t now = millis();
    bool raw = digitalRead(pin_) == LOW;   // button pulls to GND

    if (raw != down_) {
        down_ = raw;
        last_change_ = now;
    }
    if (now - last_change_ < BTN_DEBOUNCE_MS) return BTN_NONE;

    if (raw == stable_) {
        // Steady state: fire hold events and settle pending shorts.
        if (stable_) {
            if (!verylong_fired_ && now - press_start_ >= 3000) {
                verylong_fired_ = true;
                return BTN_VERYLONG;
            }
            if (!long_fired_ && now - press_start_ >= 600) {
                long_fired_ = true;
                return BTN_LONG;
            }
        } else if (waiting_double_ && now - last_release_ >= 350) {
            // No second tap came — the earlier release was a plain short press.
            waiting_double_ = false;
            return BTN_SHORT;
        }
        return BTN_NONE;
    }

    stable_ = raw;
    if (raw) {
        press_start_ = now;
        long_fired_ = false;
        verylong_fired_ = false;
        return BTN_NONE;
    }

    // Release. Holds were already reported while the button was down.
    if (long_fired_ || verylong_fired_) return BTN_NONE;
    if (waiting_double_ && now - last_release_ < 350) {
        waiting_double_ = false;
        return BTN_DOUBLE;
    }
    waiting_double_ = true;
    last_release_ = now;
    return BTN_NONE;   // becomes BTN_SHORT once the double-window closes
}
