#ifndef _OTA_H
#define _OTA_H

#define wifi_SSID "2024"     // WIFI名称，区分大小写，不要写错
#define wifi_PASS "12345678" // WIFI密码

extern int a, b;

void updateBin(); // wifiManager配网      httpUpdate OTA更新

#endif