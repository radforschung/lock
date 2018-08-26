#include "globals.h"

Location::Location() {
	ESP_LOGD(TAG, "init location");
	//Prepare Wifi
	WiFi.mode(WIFI_STA);
  	WiFi.disconnect();
	  scanWifis();
}

void Location::scanWifis() {
	int networkCount = WiFi.scanNetworks();
	ESP_LOGD(TAG, "scan wifis");
	for (int i = 0; (i < networkCount); ++i) {
		ESP_LOGD(TAG, "wifinetwork=%i", i);
		String bssid = WiFi.BSSIDstr(i);
		
		ESP_LOGD(TAG, "wifibssid=%s", bssid.c_str());
		ESP_LOGD(TAG, "wifisssid=%s", WiFi.SSID(i).c_str());
		ESP_LOGD(TAG, "wifirssi=%i", WiFi.RSSI(i));
	}
}