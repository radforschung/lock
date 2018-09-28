#include "globals.h"
#include "rfid.h"

Adafruit_PN532 nfc(PN532_SS);

Rfid::Rfid() {
  
  ESP_LOGD(TAG, "init RFID");
  // Prepare RFID
	Serial.println("rfid");
  nfc.begin();
    uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
		ESP_LOGD(TAG, "cant find RFID board");
    //while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  // configure board to read RFID tags
  nfc.SAMConfig();

  Serial.println("Waiting for an ISO14443A Card ...");
  
}

void Rfid::checkCard() {
	ESP_LOGD(TAG, "check Card");
	Serial.println("check Card");
	uint8_t success;                          // Flag to check if there was an error with the PN532
	uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
	uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
	Serial.println("before check");
	success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
	Serial.println("after check");
	if (success) {
		Serial.println("Found an ISO14443A card");
		Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
		Serial.print("  UID Value: ");
		nfc.PrintHex(uid, uidLength);
		Serial.println("");
	}
	Serial.println("check Card END");
}