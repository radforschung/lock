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
