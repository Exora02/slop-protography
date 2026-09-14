#pragma once

#include <Arduino.h>

// Wi-Fi + naming. The device is its own access point by default: phones join
// "Protography-XXXX" (open) and get redirected to the UI by the captive DNS.
// All traffic stays between phone and camera — no internet needed.

bool  net_begin();                  // start AP (+ captive DNS + mDNS)
void  net_dns_tick();               // service captive-portal DNS queries
const String& net_ap_ssid();        // effective SSID
IPAddress net_ap_ip();              // 192.168.4.1
String net_url();                   // http://protography.local hint
