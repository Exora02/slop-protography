#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// WebSocket server (port 81): live view + push events + a few commands.
//
// Binary frames: 0x01 prefix + JPEG bytes (live view frames).
// Text frames: JSON. Commands from the phone are dispatched through the
// callback registered by main (keeps this module free of capture deps);
// events to the phone are sent with ws_broadcast_json().

using WsCommandHandler = void (*)(JsonVariant cmd);

bool  ws_begin(WsCommandHandler handler);
void  ws_loop();
bool  ws_send_jpeg(const uint8_t* buf, size_t len);
void  ws_broadcast_json(JsonDocument& doc);
void  ws_broadcast_json_str(const String& json);
bool  ws_has_clients();
uint8_t ws_client_count();
void  ws_note_client(bool connected);   // for LED/activity tracking
