#include <Arduino.h>
#include <Bounce2.h>
#include "lock.h"

//#define USE_MODEL_NODEMCU32
#define USE_MODEL_TTGOV2

#ifdef USE_MODEL_NODEMCU32
  #define PIN_LOCK_MOTOR GPIO_NUM_14
  #define PIN_LOCK_ROTATION_SWITCH GPIO_NUM_12 // (PULLUP!)  LOW: default, HIGH: motor presses against (rotation complete)
  #define PIN_LOCK_LATCH_SWITCH GPIO_NUM_13 // (PULLUP!) LOW: closed, HIGH: open
#endif
#ifdef USE_MODEL_TTGOV2
  #define PIN_LOCK_MOTOR GPIO_NUM_15
  #define PIN_LOCK_ROTATION_SWITCH GPIO_NUM_2 // (PULLUP!)  LOW: default, HIGH: motor presses against (rotation complete)
  #define PIN_LOCK_LATCH_SWITCH GPIO_NUM_4 // (PULLUP!) LOW: closed, HIGH: open
#endif


Lock lock;

void setup() {
  Serial.begin(115200);
  lock = Lock();

  Serial.println("msg='hello world' version=0.0.2");
}

void loop() {
  if (!lock.isOpen()) {
    Serial.println("closed");
    lock.open();
  } else {
    delay(1000);
    if (!lock.motorIsParked()){
      lock.open();
    }
    Serial.println("is Open");
  }
}
