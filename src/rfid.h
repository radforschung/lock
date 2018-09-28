#define SPI_HAS_TRANSACTION 1
#include <Adafruit_PN532.h>

#define PN532_SCK  (5)
#define PN532_MOSI (27)
#define PN532_SS   (23)
#define PN532_MISO (19)

class Rfid {
	public:
		Rfid();
		void checkCard();
};
