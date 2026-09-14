#include "net.h"

#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>

#include "log.h"
#include "settings.h"

static const char* TAG = "net";

static DNSServer s_dns;
static String s_ssid;
static bool s_dns_up = false;

bool net_begin() {
    // Effective SSID: user override or "Protography-" + last 2 MAC bytes.
    if (g_settings.ap_name[0] != '\0') {
        s_ssid = g_settings.ap_name;
    } else {
        uint64_t mac = ESP.getEfuseMac();
        char suffix[6];
        snprintf(suffix, sizeof(suffix), "%02X%02X", (unsigned)(mac >> 8) & 0xFF,
                 (unsigned)mac & 0xFF);
        s_ssid = String("Protography-") + suffix;
    }

    WiFi.persistent(false);
    WiFi.mode(WIFI_AP);
    bool ok;
    if (g_settings.ap_pass[0] != '\0') {
        ok = WiFi.softAP(s_ssid.c_str(), g_settings.ap_pass);
    } else {
        // Open network: instant to join on a phone, which fits an art camera.
        // Set ap_pass in settings if you'd rather have WPA2.
        ok = WiFi.softAP(s_ssid.c_str());
    }
    if (!ok) {
        LOGE(TAG, "softAP start failed");
        return false;
    }
    WiFi.softAPConfig(AP_DEFAULT_IP, AP_DEFAULT_IP, IPAddress(255, 255, 255, 0));
    LOGI(TAG, "AP \"%s\" at %s", s_ssid.c_str(), WiFi.softAPIP().toString().c_str());

    // Captive portal: any DNS query resolves to us.
    s_dns_up = s_dns.start(DNS_PORT, "*", AP_DEFAULT_IP);

    if (MDNS.begin(MDNS_HOSTNAME)) {
        MDNS.addService("http", "tcp", HTTP_PORT);
    } else {
        LOGW(TAG, "mDNS start failed (UI still on IP)");
    }
    return true;
}

void net_dns_tick() {
    if (s_dns_up) s_dns.processNextRequest();
}

const String& net_ap_ssid() { return s_ssid; }
IPAddress net_ap_ip() { return WiFi.softAPIP(); }
String net_url() { return String("http://") + MDNS_HOSTNAME + ".local"; }
