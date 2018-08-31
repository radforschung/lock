#include <vector>
#include "globals.h"
#include "location.h"

static const char *TAG = "location";

// wifis which contain this string did
// opt out from location services
static const String nomapSuffix = "_nomap";
static const int maxSendWifis = 7;

Location::Location() {
  ESP_LOGD(TAG, "init location");
  // Prepare Wifi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
}

void Location::scanWifis() {
  int networkCount = WiFi.scanNetworks();
  ESP_LOGD(TAG, "msg=\"scanned wifis\" count=%d", networkCount);

  std::vector<uint8_t> message = {0x02};
  int sendWifiCount = 0;
  for (int i = 0; i < networkCount; ++i) {
    String ssid = WiFi.SSID(i);
    // ignore opt outed wifi networks
    if (strstr(ssid.c_str(), nomapSuffix.c_str())) {
      continue;
    }
    if (sendWifiCount >= maxSendWifis) {
      break;
    }
    String bssid = WiFi.BSSIDstr(i);
    int rssi = WiFi.RSSI(i);
    ESP_LOGD(TAG, "wifibssid=%s wifisssid=%s wifirssi=%i", bssid.c_str(),
             ssid.c_str(), rssi);
    uint8_t *network = WiFi.BSSID(i);
    for (int j = 0; j < 6; ++j) {
      message.push_back(network[j]);
    }
    message.push_back(rssi * -1);
    sendWifiCount++;
  }

  String result = "";
  char buff[16];
  for (int j = 0; j < message.size(); ++j) {
    snprintf(buff, sizeof(buff), "%x", message.at(j));
    result = result + " 0x" + buff;
  }
  ESP_LOGD(TAG, "size=%i result=%s", message.size(), result.c_str());

  loraSend(LORA_PORT_LOCATION_WIFI, message.data(), message.size());
}
