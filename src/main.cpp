#include <Arduino.h>
#include <Bounce2.h>

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Preferences.h>

//#define USE_MODEL_NODEMCU32
#define USE_MODEL_TTGOV2

#ifdef USE_MODEL_NODEMCU32
  #define PIN_LOCK_MOTOR           GPIO_NUM_14
  #define PIN_LOCK_ROTATION_SWITCH GPIO_NUM_12 // (PULLUP!)  LOW: default, HIGH: motor presses against (rotation complete)
  #define PIN_LOCK_LATCH_SWITCH    GPIO_NUM_13 // (PULLUP!) LOW: closed, HIGH: open
#endif
#ifdef USE_MODEL_TTGOV2
  #define PIN_LOCK_MOTOR           GPIO_NUM_15
  #define PIN_LOCK_ROTATION_SWITCH GPIO_NUM_2 // (PULLUP!)  LOW: default, HIGH: motor presses against (rotation complete)
  #define PIN_LOCK_LATCH_SWITCH    GPIO_NUM_4 // (PULLUP!) LOW: closed, HIGH: open

  #define PIN_SPI_SS    GPIO_NUM_18 // HPD13A NSS/SEL (Pin4) SPI Chip Select Input
  #define PIN_SPI_MOSI  GPIO_NUM_27 // HPD13A MOSI/DSI (Pin6) SPI Data Input
  #define PIN_SPI_MISO  GPIO_NUM_19 // HPD13A MISO/DSO (Pin7) SPI Data Output
  #define PIN_SPI_SCK   GPIO_NUM_5  // HPD13A SCK (Pin5) SPI Clock Input

  #define PIN_RST   LMIC_UNUSED_PIN // connected to ESP32 RST/EN
  #define PIN_DIO0  GPIO_NUM_26     // ESP32 GPIO26 wired on PCB to HPD13A
  #define PIN_DIO1  GPIO_NUM_33     // HPDIO1 on pcb, needs to be wired external to GPIO33
  #define PIN_DIO2  LMIC_UNUSED_PIN // 32 HPDIO2 on pcb, needs to be wired external to GPIO32 (not necessary for LoRa, only FSK)
#endif

#include <config.h>

const unsigned TX_INTERVAL = 60;

void os_getArtEui (u1_t* buf) {}
void os_getDevEui (u1_t* buf) {}
void os_getDevKey (u1_t* buf) {}

const lmic_pinmap lmic_pins = {
  .nss = PIN_SPI_SS,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = PIN_RST,
  .dio = {PIN_DIO0, PIN_DIO1, PIN_DIO2}
};

static osjob_t sendjob;
Preferences preferences;

bool firstBoot = true;

Bounce debounceRotationSwitch = Bounce();
Bounce debounceLatchSwitch = Bounce();

bool shouldRotate = false;

void setupLoRa() {
  // init spi before
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, 0x00);

  // LMIC init
  os_init();

  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  // Set static session parameters.
  // copy for esp
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession(0x1, DEVADDR, nwkskey, appskey);

  // setup EU channels
  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band

  // Disable link check validation
  LMIC_setLinkCheckMode(0);

  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;

  // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
  int txpower = 14;
  LMIC_setDrTxpow(DR_SF12, txpower);

  LMIC.seqnoUp = preferences.getUInt("counter", 1);
  Serial.printf("time=%.3f loraseq=%d\n", ((float) millis() / 1000.0), LMIC.seqnoUp);
}


void do_send(osjob_t* j){
  // Payload to send (uplink)
  uint8_t message[] = {0x01, 0x00, 0x00};
  if (debounceLatchSwitch.read() == LOW) {
    message[1] = 0x01;
  } else {
    message[1] = 0x02;
  }

  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.printf("time=%.3f msg=\"OP_TXRXPEND, not sending\" loraseq=%d\n", ((float) millis() / 1000.0), LMIC.seqnoUp);
  } else {
    // Prepare upstream data transmission at the next possible time.
    LMIC_setTxData2(1, message, sizeof(message)-1, 0);
    preferences.putUInt("counter", LMIC.seqnoUp);
    Serial.printf("time=%.3f msg=\"sending packet\" loraseq=%d\n", ((float) millis() / 1000.0), LMIC.seqnoUp);
  }
  // Next TX is scheduled after TX_COMPLETE event.
  os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  preferences.begin("lock32", false);

  pinMode(PIN_LOCK_LATCH_SWITCH, INPUT_PULLUP);
  pinMode(PIN_LOCK_ROTATION_SWITCH, INPUT_PULLUP);
  pinMode(PIN_LOCK_MOTOR, OUTPUT);

  debounceRotationSwitch.attach(PIN_LOCK_ROTATION_SWITCH);
  debounceRotationSwitch.interval(5);

  debounceLatchSwitch.attach(PIN_LOCK_LATCH_SWITCH);
  debounceLatchSwitch.interval(5);

  setupLoRa();

  Serial.println("msg=\"hello world\" version=0.0.1");

  os_setCallback(&sendjob, do_send);
}

void onEvent (ev_t ev) {
  Serial.printf("time=%.3f", ((float) millis() / 1000.0));
  switch(ev) {
    case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(" loraEvent=EV_RXCOMPLETE");
      break;
  }
  if (ev == EV_TXCOMPLETE) {
    Serial.println(" loraEvent=EV_TXCOMPLETE");
    if (LMIC.txrxFlags & TXRX_ACK) {
      Serial.println(" msg=\"Received ack\"");
    }
    if (LMIC.dataLen) {
      Serial.print(" msg=\"Received\" payloadLength=");
      Serial.print(LMIC.dataLen);
      Serial.print(F(" data=0x"));
      for (int i = 0; i < LMIC.dataLen; i++) {
        if (LMIC.frame[LMIC.dataBeg + i] < 0x10) {
          Serial.print(F("0"));
        }
        Serial.print(LMIC.frame[LMIC.dataBeg + i], HEX);
      }
      Serial.println();
    }
    // Schedule next transmission
    os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
  }
}

void loop() {
  os_runloop_once();

  debounceRotationSwitch.update();
  debounceLatchSwitch.update();

  if (debounceRotationSwitch.read() == HIGH) {
    shouldRotate = false;
  }

  if (debounceLatchSwitch.read() == LOW) {
    shouldRotate = true;
  }

  if (firstBoot || millis() % 1000 == 0) {
    firstBoot = false;
    Serial.printf("time=%.3f rotation=%d latch=%d shouldRotate=%d\n",
      ((float) millis() / 1000.0),
      debounceRotationSwitch.read(),
      debounceLatchSwitch.read(),
      shouldRotate);
  }

  if (shouldRotate) {
    digitalWrite(PIN_LOCK_MOTOR, HIGH);
  } else {
    digitalWrite(PIN_LOCK_MOTOR, LOW);
  }
}
