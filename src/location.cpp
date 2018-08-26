#include "globals.h"
#include <vector>

Location::Location() {
	ESP_LOGD(TAG, "init location");
	//Prepare Wifi
	WiFi.mode(WIFI_STA);
  	WiFi.disconnect();
}

void Location::scanWifis() {
	String nomapString = "_nomap"; //Wifis which contain this string are opt outed of location services
	int maxSendWifis = 7;
	int networkCount = WiFi.scanNetworks();
	ESP_LOGD(TAG, "scan wifis");
	std::vector<uint8_t> message = { 0x02, 0x01 };
	int sendWifiCount = 0;
	for (int i = 0; (i < networkCount); ++i) {
		String ssid = WiFi.SSID(i);
		//ignore opt outed wifi networks
		if (!strstr(ssid.c_str(), nomapString.c_str())) {
			if (sendWifiCount >= maxSendWifis) {
				break;
			}
			String bssid = WiFi.BSSIDstr(i);
			ESP_LOGD(TAG, "wifibssid=%s", bssid.c_str());
			ESP_LOGD(TAG, "wifisssid=%s", ssid.c_str());
			ESP_LOGD(TAG, "wifirssi=%i", WiFi.RSSI(i));
			uint8_t *network = WiFi.BSSID(i);
			for (int j=0; j < 6; ++j) {
				message.push_back(network[j]);
			}
			message.push_back(WiFi.RSSI(i)*-1);
			sendWifiCount++;
		}
	}
	String result = "";
	char buff[16];
	for (int j=0; j < message.size(); ++j) {
		snprintf(buff, sizeof(buff), "%x", message.at(j));
		result = result + " 0x" + buff;
	}
	//uint8_t msg[] = { 0x02, 0x03, 0x05, 0xFF };
  	//loraSend(msg);
	loraSend(message.data());

	ESP_LOGD(TAG, "size=%i", message.size());
	ESP_LOGD(TAG, "result=%s", result.c_str());
}