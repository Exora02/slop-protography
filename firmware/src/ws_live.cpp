#include "ws_live.h"

#include <WebSocketsServer.h>

#include "config.h"
#include "log.h"

static const char* TAG = "ws";

static WebSocketsServer s_ws(WS_PORT);
static WsCommandHandler s_handler = nullptr;
static uint8_t s_clients = 0;

static void ws_event(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            ws_note_client(true);
            LOGI(TAG, "client %u connected", num);
            break;
        case WStype_DISCONNECTED:
            ws_note_client(false);
            LOGI(TAG, "client %u disconnected", num);
            break;
        case WStype_TEXT: {
            if (!s_handler) break;
            JsonDocument doc;
            if (deserializeJson(doc, payload, length)) {
                s_handler(doc.as<JsonVariant>());
            } else {
                LOGW(TAG, "bad json from client %u", num);
            }
            break;
        }
        default:
            break;
    }
}

bool ws_begin(WsCommandHandler handler) {
    s_handler = handler;
    s_ws.begin();
    s_ws.onEvent(ws_event);
    LOGI(TAG, "websocket server on :%u", WS_PORT);
    return true;
}

void ws_loop() { s_ws.loop(); }

void ws_note_client(bool connected) {
    if (connected) {
        if (s_clients < 255) s_clients++;
    } else {
        if (s_clients > 0) s_clients--;
    }
}

uint8_t ws_client_count() { return s_clients; }
bool ws_has_clients() { return s_clients > 0; }

bool ws_send_jpeg(const uint8_t* buf, size_t len) {
    if (s_clients == 0) return false;
    // Prepend the frame-type byte so the client can route binary messages.
    static uint8_t* tx = nullptr;
    static size_t tx_cap = 0;
    if (len + 1 > tx_cap) {
        if (tx) heap_caps_free(tx);
        tx_cap = len + 1 + 4096;
        tx = (uint8_t*)heap_caps_malloc(tx_cap, MALLOC_CAP_SPIRAM);
        if (!tx) {
            tx_cap = 0;
            return false;
        }
    }
    tx[0] = LIVE_CHUNK_PREFIX_JPEG;
    memcpy(tx + 1, buf, len);
    s_ws.broadcastBIN(tx, len + 1);
    return true;
}

void ws_broadcast_json(JsonDocument& doc) {
    String out;
    serializeJson(doc, out);
    s_ws.broadcastTXT(out);
}

void ws_broadcast_json_str(const String& json) {
    // broadcastTXT wants a mutable String&
    String copy = json;
    s_ws.broadcastTXT(copy);
}
