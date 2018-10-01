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

static void lock_isr_handler(void *args) {
  gpio_isr_handler_remove(PIN_LOCK_LATCH_SWITCH);
  xQueueSendToBackFromISR(lockQueue, &LOCK_TASK_GPIO_ISR, NULL);
}

void lock_task(void *ignore) {
  ESP_LOGD(TAG, "task=lock_task state=enter");
  Lock lock = Lock();

  // TODO clean this up, mode etc is already set in lock.cpp
  //     only thing to be done is to set INTR_ANYEDGE
  gpio_config_t gpioConfig;
  gpioConfig.pin_bit_mask = GPIO_SEL_4;
  gpioConfig.mode = GPIO_MODE_INPUT;
  gpioConfig.pull_up_en = GPIO_PULLUP_ENABLE;
  gpioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpioConfig.intr_type = GPIO_INTR_ANYEDGE;
  gpio_config(&gpioConfig);

  gpio_install_isr_service(0);
  gpio_isr_handler_add(PIN_LOCK_LATCH_SWITCH, lock_isr_handler, NULL);

  boolean lastState = false;
  int task;
  while (1) {
    // wait for activation by queue
    ESP_LOGD(TAG, "task=lock_task state=waiting");
    if (xQueueReceive(lockQueue, &task, portMAX_DELAY) != pdPASS) {
      continue;
    }
    ESP_LOGD(TAG, "task=lock_task state=active");

    if (task == LOCK_TASK_GPIO_ISR) {
      bool open = digitalRead(PIN_LOCK_LATCH_SWITCH);
      if (lastState != open) {
        ESP_LOGI(TAG, "Lock state changed: %d", open);
        xQueueSend(taskQueue, &TASK_SEND_LOCK_STATUS, portMAX_DELAY);
        xQueueSend(taskQueue, &TASK_SEND_LOCATION_WIFI, portMAX_DELAY);
        xQueueSend(taskQueue, &TASK_SEND_LOCATION_GPS, portMAX_DELAY);
      }
      vTaskDelay(100 / portTICK_PERIOD_MS);
      lastState = open;

      xQueueSendToFront(lockQueue, &LOCK_TASK_PARK, portMAX_DELAY);

      gpio_isr_handler_add(PIN_LOCK_LATCH_SWITCH, lock_isr_handler, NULL);
    } else if (task == LOCK_TASK_PARK) {
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
