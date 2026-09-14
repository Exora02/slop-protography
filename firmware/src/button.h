#pragma once

#include <Arduino.h>

#include "config.h"

// Tiny non-blocking button with debounce and short/double/long/very-long
// press detection. Poll from the main loop.

enum BtnEvent : uint8_t {
    BTN_NONE = 0,
    BTN_SHORT,     // press+release < 400 ms
    BTN_DOUBLE,    // two shorts within 350 ms
    BTN_LONG,      // held >= 600 ms (fires once while held)
    BTN_VERYLONG,  // held >= 3000 ms (fires once while held)
};

class Button {
   public:
    explicit Button(uint8_t pin);
    void begin();
    BtnEvent tick();   // call from loop(); consumes one event at most

   private:
    uint8_t  pin_;
    bool     down_ = false;
    bool     stable_ = false;
    uint32_t last_change_ = 0;
    uint32_t press_start_ = 0;
    bool     long_fired_ = false;
    bool     verylong_fired_ = false;
    uint32_t last_release_ = 0;
    bool     waiting_double_ = false;
};
