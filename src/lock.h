#ifndef _lock_lock_h
#define _lock_lock_h

#define PIN_LOCK_MOTOR GPIO_NUM_15
// rotation switch: (PULLUP!) LOW: default, HIGH: motor presses against
// (rotation complete)
#define PIN_LOCK_ROTATION_SWITCH GPIO_NUM_2

class Lock {
public:
  Lock();
  void open();
  bool isOpen();
  bool motorIsParked();

private:
  Bounce debounceRotationSwitch;
  Bounce debounceLatchSwitch;
};

#endif // _lock_lock_h
