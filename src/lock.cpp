#include "globals.h"
#include "lock.h"

static const char *TAG = "lock";

QueueHandle_t lockQueue = NULL;

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

void lock_task(void *ignore) {
  ESP_LOGD(TAG, "task=lock_task state=enter");
  Lock lock = Lock();
  int task;
  while (1) {
    // wait for activation by queue
    ESP_LOGD(TAG, "task=lock_task state=waiting");
    if (xQueueReceive(lockQueue, &task, portMAX_DELAY) != pdPASS) {
      continue;
    }
    ESP_LOGD(TAG, "task=lock_task state=active");

    if (task == LOCK_TASK_PARK) {
      ESP_LOGD(TAG, "task=lock_task action=park");
      if (!lock.motorIsParked()) {
        lock.open();
      }
    } else if (task == LOCK_TASK_UNLOCK) {
      ESP_LOGD(TAG, "task=lock_task action=unlock");
      lock.open();
    } else if (task == LOCK_TASK_SEND_STATUS) {
      ESP_LOGD(TAG, "task=lock_task action=send");
      uint8_t msg[] = {0x01, (uint8_t)((!lock.isOpen()) ? 0x01 : 0x02)};
      loraSend(LORA_PORT_LOCK_STATUS, msg, sizeof(msg));
    }
    task = 0;
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}
