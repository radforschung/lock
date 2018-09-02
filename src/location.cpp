#include "globals.h"
#include "location.h"

// wifis which contain this string did
// opt out from location services
static const String nomapSuffix = "_nomap";
static const int maxScanWifis = 7;

Location::Location() {
  log_d("init location");
  // Prepare Wifi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
}

std::vector<uint8_t> Location::scanWifis() {
  int networkCount = WiFi.scanNetworks();
  log_d("msg=\"scanned wifis\" count=%d", networkCount);

  std::vector<uint8_t> message = {0x02};
  int scanWifiCount = 0;
  for (int i = 0; i < networkCount; ++i) {
    String ssid = WiFi.SSID(i);
    // ignore opt outed wifi networks
    if (strstr(ssid.c_str(), nomapSuffix.c_str())) {
      continue;
    }
    if (scanWifiCount >= maxScanWifis) {
      break;
    }
    String bssid = WiFi.BSSIDstr(i);
    int rssi = WiFi.RSSI(i);
    log_d("wifibssid=%s wifisssid=%s wifirssi=%i", bssid.c_str(),
            ssid.c_str(), rssi);
    uint8_t *network = WiFi.BSSID(i);
    for (int j = 0; j < 6; ++j) {
      message.push_back(network[j]);
    }
    message.push_back(rssi * -1);
    scanWifiCount++;
  }

  String result = "";
  char buff[16];
  for (int j = 0; j < message.size(); ++j) {
    snprintf(buff, sizeof(buff), "%x", message.at(j));
    result = result + " 0x" + buff;
  }
  log_d("size=%i result=%s", message.size(), result.c_str());

  return message;
}
