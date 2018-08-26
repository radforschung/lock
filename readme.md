radforschung lock
=================

ESP32 based smart lock for bicycles and other shared mobility devices.

This is a work in progress. For more background see https://radforschung.org/log


## hardware prototype

for the prototype we're using an modified smart lock from china. as platform for the esp32 and lora a ttgo lora v2 is used.
![](hardware/prototype-ttgo-v2.svg)

## Lora

### Lora config

rename `src/config.sample.h` to `src/config.h` and insert your ttn application credentials.

### Lora messages

#### Lock Status

Lock messages start with `0x01`. The Second Byte is the Status.
 * `0x01` `0x01`: Lock Closed
 * `0x01` `0x02`: Lock Open

#### Location Info

Location Info starts with `0x02`. There Are currently two Location reporting methods: GPS and Wifi. While GPS receives the location on the lock, wifi location can't calculated on the lock.

##### GPS

tbd

##### WiFi

Second byte `0x02`. The following bytes containes chunks of 7 bytes. the first 6 bytes containes the WIFIs BSSID (MAC-address) of the wifi, and the 7th byte is the RSSI*-1. Max number of wifis is 7.
