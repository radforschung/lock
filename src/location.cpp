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
    if (xQueueReceive(wifiQueue, &noMessage, portMAX_DELAY) != pdTRUE) {
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
    if (xQueueReceive(gpsQueue, &noMessage, portMAX_DELAY) != pdTRUE) {
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
      uint8_t txBuffer[10];
      uint32_t latBin, lngBin;
      uint16_t alt;
      uint8_t hdop;

      latBin = ((gps.location.lat() + 90) / 180) * 16777215;
      lngBin = ((gps.location.lng() + 180) / 360) * 16777215;

      txBuffer[0] = (latBin >> 16) & 0xFF;
      txBuffer[1] = (latBin >> 8) & 0xFF;
      txBuffer[2] = latBin & 0xFF;

      txBuffer[3] = (lngBin >> 16) & 0xFF;
      txBuffer[4] = (lngBin >> 8) & 0xFF;
      txBuffer[5] = lngBin & 0xFF;

      alt = gps.altitude.meters();
      txBuffer[6] = (alt >> 8) & 0xFF;
      txBuffer[7] = alt & 0xFF;

      hdop = gps.hdop.hdop() * 10;
      txBuffer[8] = hdop & 0xFF;

      txBuffer[9] = gps.satellites.value() & 0xFF;

      ESP_LOGI(TAG, "gps=\"%x %x %x %x %x %x %x %x %x %x\"", txBuffer[0],
               txBuffer[1], txBuffer[2], txBuffer[3], txBuffer[4], txBuffer[5],
               txBuffer[6], txBuffer[7], txBuffer[8], txBuffer[9]);

      for (size_t i = 0; i <= 9; i++) {
        message.push_back(txBuffer[i]);
      }
    }
    // no valid GPS location:
    else {
      ESP_LOGI(TAG, "gps=\"invalid\"");
      for (size_t i = 0; i <= 9; i++) {
        message.push_back(0);
      }
    }

    // on every loop:
    if (gps.time.isValid() && gps.time.isUpdated()) {
      ESP_LOGI(TAG, "gps_time=%d", gps.time.value());
    }
    ESP_LOGD(TAG, "chars=%i passed=%i", gps.charsProcessed(),
             gps.passedChecksum());

    // FIXME: send queue: message
    loraSend(LORA_PORT_LOCATION_GPS, message.data(), message.size());
  }
  vTaskDelete(NULL);
}
