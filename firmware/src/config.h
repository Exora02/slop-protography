#pragma once

// ---------------------------------------------------------------------------
// Protography — compile-time configuration
// ---------------------------------------------------------------------------

#ifndef PROTO_FW_VERSION
#define PROTO_FW_VERSION "0.1.0-dev"
#endif

// ---- Camera pins (XIAO ESP32S3 Sense camera connector, OV5640 module) ------
// Source: Seeed Studio wiki — XIAO ESP32S3 Sense camera example.
// The camera rides the dedicated B2B connector, so the edge header stays free
// for the shutter button.
#define CAM_PIN_PWDN    (-1)
#define CAM_PIN_RESET   (-1)
#define CAM_PIN_XCLK    (10)
#define CAM_PIN_SIOD    (40)   // SCCB (I2C) data
#define CAM_PIN_SIOC    (39)   // SCCB (I2C) clock
#define CAM_PIN_Y9      (48)
#define CAM_PIN_Y8      (11)
#define CAM_PIN_Y7      (12)
#define CAM_PIN_Y6      (14)
#define CAM_PIN_Y5      (16)
#define CAM_PIN_Y4      (18)
#define CAM_PIN_Y3      (17)
#define CAM_PIN_Y2      (15)
#define CAM_PIN_VSYNC   (38)
#define CAM_PIN_HREF    (47)
#define CAM_PIN_PCLK    (13)

// ---- Shutter button --------------------------------------------------------
// Wire the button between D1 (GPIO2) and GND. Internal pull-up at runtime.
// For deep-sleep wake, add an external 100k pull-up to 3V3 (internal pulls
// are not guaranteed to be retained in deep sleep on the S3).
#define BTN_PIN_SHUTTER (2)
#define BTN_DEBOUNCE_MS (30)

// ---- Status LED -------------------------------------------------------------
// XIAO ESP32S3 user LED is on GPIO21 and active LOW. If a board revision
// turns out to be active high, flip LED_ACTIVE_LOW to 0.
// NOTE: on the Sense expansion board, the microSD chip-select is ALSO GPIO21
// (Seeed's design). When an SD card is mounted the LED is handed over to the
// card and flickers on writes — the firmware does this automatically.
#define LED_PIN         (21)
#define LED_ACTIVE_LOW  (1)

// ---- SD card (persistent storage tier) ---------------------------------------
// Defaults match the microSD slot on the XIAO ESP32S3 Sense expansion board
// (FAT32, <=32 GB, gold fingers inward). Wiring your own slot? Move CS to a
// free pin (e.g. GPIO4 / D3) and the status LED stays yours.
#define SD_PIN_CS       (21)
#define SD_PIN_SCK      (7)    // D8
#define SD_PIN_MISO     (8)    // D9
#define SD_PIN_MOSI     (9)    // D10
#define SD_SPI_HZ       (20000000)
#define SD_LIST_MAX     (128)  // cap on SD-side photos listed in the gallery

// ---- Network ----------------------------------------------------------------
#define HTTP_PORT       (80)
#define WS_PORT         (81)
#define DNS_PORT        (53)
#define AP_DEFAULT_IP   IPAddress(192, 168, 4, 1)
#define MDNS_HOSTNAME   "protography"

// ---- Photo storage (PSRAM arena) --------------------------------------------
// Total budget for captured stills living in PSRAM. Older photos are evicted
// (LRU) when a new capture does not fit.
#define PHOTO_ARENA_BYTES   (5u * 1024u * 1024u)
#define PHOTO_MAX_COUNT     (32)

// ---- Live view ---------------------------------------------------------------
#define LIVE_DEFAULT_FPS        (12)
#define LIVE_MAX_FPS            (24)
#define LIVE_CHUNK_PREFIX_JPEG  (0x01)   // first byte of WS binary frames

// ---- Power management ---------------------------------------------------------
#define SLEEP_DEFAULT_MIN       (10)     // idle minutes before deep sleep (0 = never)
