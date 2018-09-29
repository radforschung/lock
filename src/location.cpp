#include "globals.h"
#include "location.h"

#include <TinyGPS++.h>
#include <WiFi.h>

static const char *TAG = "location";

QueueHandle_t wifiQueue = NULL;
QueueHandle_t gpsQueue = NULL;

std::vector<uint8_t> scanWifis() {
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

void wifi_task(void *ignore) {
  ESP_LOGD(TAG, "task=wifi_task state=enter");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  void *noMessage;
  while (1) {
    // wait for activation by queue
    ESP_LOGD(TAG, "task=wifi_task state=waiting");
    if (xQueueReceive(wifiQueue, &noMessage, portMAX_DELAY) != pdPASS) {
      continue;
    }
    ESP_LOGD(TAG, "task=wifi_task state=active");

    std::vector<uint8_t> msg = scanWifis();
    // FIXME: send to send / idle queue
    loraSend(LORA_PORT_LOCATION_WIFI, msg.data(), msg.size());
  }
  vTaskDelete(NULL);
}

void gps_task(void *ignore) {
  ESP_LOGD(TAG, "task=wifi_task state=enter");
  TinyGPSPlus gps;
  HardwareSerial Serial2(1);
  Serial2.begin(GPSBaud, SERIAL_8N1, GPSRX, GPSTX);
  void *noMessage;
  while (1) {
    // wait for activation by queue
    ESP_LOGD(TAG, "task=gps_task state=waiting");
    if (xQueueReceive(gpsQueue, &noMessage, portMAX_DELAY) != pdPASS) {
      continue;
    }
    ESP_LOGD(TAG, "task=gps_task state=active");

    long now = millis();
    do {
      while (Serial2.available() > 0) {
        gps.encode(Serial2.read());
      }
    } while ((millis() - now) < 3500);

    std::vector<uint8_t> message = {0x02};

    // Valid GPS location:
    if (gps.location.isValid() && gps.location.isUpdated()) {
      int32_t latitude = gps.location.lat() * 10000;
      int32_t longitude = gps.location.lng() * 10000;

      ESP_LOGI(TAG, "GPS fix, Lat: %ld, Lon: %lu, Sats: %d", latitude,
               longitude, gps.satellites.value());

      message.push_back(latitude);
      message.push_back(longitude);
      message.push_back(gps.satellites.value());
    }
    // no valid GPS location:
    else {
      ESP_LOGI(TAG, "No valid GPS location");
      message.push_back(0);
      message.push_back(0);
      message.push_back(0);
    }

    // on every loop:
    if (gps.time.isValid() && gps.time.isUpdated()) {
      ESP_LOGI(TAG, "GPS time: %d", gps.time.value());
      message.push_back(gps.time.value());
    } else {
      message.push_back(0);
    }
    ESP_LOGD(TAG, "GPS chars processed: %i, passed checksum: %i",
             gps.charsProcessed(), gps.passedChecksum());

    // FIXME: send queue: message
    loraSend(LORA_PORT_LOCATION_GPS, message.data(), message.size());
  }
  vTaskDelete(NULL);
}
