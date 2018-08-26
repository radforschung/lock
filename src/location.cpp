#include "globals.h"

Location::Location() {
	ESP_LOGD(TAG, "init location");
	//Prepare Wifi
	WiFi.mode(WIFI_STA);
  	WiFi.disconnect();
	  scanWifis();
}

void Location::scanWifis() {
	String nomapString = "_nomap"; //Wifis which contain this string are opt outed of location services
	int networkCount = WiFi.scanNetworks();
	ESP_LOGD(TAG, "scan wifis");
	for (int i = 0; (i < networkCount); ++i) {
		String ssid = WiFi.SSID(i);
		//ignore opt outed wifi networks
		if (!strstr(ssid.c_str(), nomapString.c_str())) {
			String bssid = WiFi.BSSIDstr(i);
			ESP_LOGD(TAG, "wifibssid=%s", bssid.c_str());
			ESP_LOGD(TAG, "wifisssid=%s", ssid.c_str());
			ESP_LOGD(TAG, "wifirssi=%i", WiFi.RSSI(i));
		}
	}
}