#pragma once

#include <Arduino.h>

// Idle-based deep sleep. The camera has no screen, so the LED going dark is
// the only sign — pressing the shutter wakes it (deep-sleep wake resets the
// chip and boots fresh into the same ready state).

void power_note_activity();   // any button/WS/HTTP traffic resets the timer
void power_tick();            // call from loop()
void power_sleep_now();       // enter deep sleep immediately
bool power_woken_by_button();
