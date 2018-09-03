#include "globals.h"
#include "location.h"
#include "WiFi.h"
#include <TinyGPS++.h>

static const char *TAG = "location";

Location::Location() {
  ESP_LOGD(TAG, "init location");
  // Prepare Wifi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
}

std::vector<uint8_t> Location::scanWifis() {
  int networkCount = WiFi.scanNetworks();
  ESP_LOGD(TAG, "msg=\"scanned wifis\" count=%d", networkCount);

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
    ESP_LOGD(TAG, "wifibssid=%s wifisssid=%s wifirssi=%i", bssid.c_str(),
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
  ESP_LOGD(TAG, "size=%i result=%s", message.size(), result.c_str());

  return message;
}

void gps_task(void *ignore) {
  ESP_LOGD(TAG, ">> gps_task");
  TinyGPSPlus gps;
  HardwareSerial Serial2(1);
  Serial2.begin(GPSBaud, SERIAL_8N1, GPSRX, GPSTX);
  while(1) {
    long now = millis();
    do {
      while (Serial2.available()>0) {
          gps.encode(Serial2.read());
      }
    } while ((millis() - now) < 3500);

    // Valid GPS location:
    if (gps.location.isValid() && gps.location.isUpdated()) {
      int32_t latitude = gps.location.lat() * 10000;
      int32_t longitude = gps.location.lng() * 10000;

      ESP_LOGI(TAG, "GPS fix, Lat: %ld, Lon: %lu, Sats: %d", latitude, longitude, gps.satellites.value());
    }
    // no valid GPS location:
    else {
      ESP_LOGI(TAG, "No valid GPS location");
    }

    // on every loop:
    if (gps.time.isValid() && gps.time.isUpdated()) {
      ESP_LOGI(TAG, "GPS time: %d", gps.time.value());
    }
    ESP_LOGD(TAG, "GPS chars processed: %i, passed checksum: %i", gps.charsProcessed(), gps.passedChecksum());

    // Go and lay dormant
    vTaskDelay(25000 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}
