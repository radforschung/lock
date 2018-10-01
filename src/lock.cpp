#include "globals.h"
#include "lock.h"

static const char *TAG = "lock";

Lock::Lock() {
  debounceRotationSwitch = Bounce();
  debounceLatchSwitch = Bounce();

  pinMode(PIN_LOCK_LATCH_SWITCH, INPUT_PULLUP);
  pinMode(PIN_LOCK_ROTATION_SWITCH, INPUT_PULLUP);
  pinMode(PIN_LOCK_MOTOR, OUTPUT);

  debounceRotationSwitch.attach(PIN_LOCK_ROTATION_SWITCH);
  debounceRotationSwitch.interval(5);

  debounceLatchSwitch.attach(PIN_LOCK_LATCH_SWITCH);
  debounceLatchSwitch.interval(5);
}

void Lock::open() {
  while (!isOpen()) {
    digitalWrite(PIN_LOCK_MOTOR, HIGH);
  }
  while (!motorIsParked()) {
    digitalWrite(PIN_LOCK_MOTOR, HIGH);
  }
  digitalWrite(PIN_LOCK_MOTOR, LOW);
}

bool Lock::isOpen() {
  debounceLatchSwitch.update();
  debugSwitch(1, "latchSwitch", debounceLatchSwitch.read());
  return (debounceLatchSwitch.read() == HIGH);
}

bool Lock::motorIsParked() {
  debounceRotationSwitch.update();
  debugSwitch(2, "rotationSwitch", debounceRotationSwitch.read());
  return debounceRotationSwitch.read() == HIGH;
}

static long lastLockDebugCall = 0;
static long lastLockDebugSource = 0;

void Lock::debugSwitch(int source, char *txt, int readout) {
  if ((millis() - lastLockDebugCall) > 500 || source != lastLockDebugSource) {
    ESP_LOGD(TAG, "%s=%d", txt, readout);
    lastLockDebugCall = millis();
    lastLockDebugSource = source;
  }
}
