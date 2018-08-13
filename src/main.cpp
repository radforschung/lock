#include <Arduino.h>
#include <Bounce2.h>

#define PIN_LOCK_MOTOR 14
#define PIN_LOCK_ROTATION_SWITCH 12 // (PULLUP!)  LOW: default, HIGH: motor presses against (rotation complete)
#define PIN_LOCK_LATCH_SWITCH 13  // (PULLUP!) LOW: closed, HIGH: open

bool firstBoot = true;

Bounce debounceRotationSwitch = Bounce();
Bounce debounceLatchSwitch = Bounce();

bool shouldRotate = false;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LOCK_LATCH_SWITCH, INPUT_PULLUP);
  pinMode(PIN_LOCK_ROTATION_SWITCH, INPUT_PULLUP);
  pinMode(PIN_LOCK_MOTOR, OUTPUT);

  debounceRotationSwitch.attach(PIN_LOCK_ROTATION_SWITCH);
  debounceRotationSwitch.interval(5);

  debounceLatchSwitch.attach(PIN_LOCK_LATCH_SWITCH);
  debounceLatchSwitch.interval(5);

  Serial.println('msg="hello world" version=0.0.1');
}

void loop() {
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
    float sec = (float) millis() / 1000.0;
    Serial.printf("time=%.3f rotation=%d latch=%d shouldRotate=%d\n",
      sec,
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
