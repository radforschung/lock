#import "lock.h"
#include <Arduino.h>
#include <Bounce2.h>

//Bounce debounceRotationSwitch = Bounce();
//Bounce debounceLatchSwitch = Bounce();

Lock::Lock() {
	Lock::debounceRotationSwitch = Bounce();
	Lock::debounceLatchSwitch = Bounce();
	
	pinMode(PIN_LOCK_LATCH_SWITCH, INPUT_PULLUP);
	pinMode(PIN_LOCK_ROTATION_SWITCH, INPUT_PULLUP);
	pinMode(PIN_LOCK_MOTOR, OUTPUT);

	Lock::debounceRotationSwitch.attach(PIN_LOCK_ROTATION_SWITCH);
	Lock::debounceRotationSwitch.interval(5);

	Lock::debounceLatchSwitch.attach(PIN_LOCK_LATCH_SWITCH);
	Lock::debounceLatchSwitch.interval(5);
}

void Lock::open() {
	while(!Lock::isOpen()) {
		digitalWrite(PIN_LOCK_MOTOR, HIGH);
	}
	digitalWrite(PIN_LOCK_MOTOR, LOW);
}

bool Lock::isOpen() {
	return (Lock::debounceLatchSwitch.read() == LOW);
}