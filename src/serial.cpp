#include "globals.h"

static const char *TAG = "serial";

void processSerial() {
  void *nothing;

  while (Serial.available()) {
    char inChar = (char)Serial.read();
    switch (inChar) {
    case '\r':
      break;
    case '\n':
      Serial.println(); // helpful for debugging
      break;
    case 'u':
      xQueueSend(taskQueue, &TASK_UNLOCK, portMAX_DELAY);
      break;
    case 'r':
      xQueueSend(taskQueue, &TASK_RESTART, portMAX_DELAY);
      break;
    case 's':
      xQueueSend(taskQueue, &TASK_SEND_LOCK_STATUS, portMAX_DELAY);
      break;
    case 'w':
      xQueueSend(wifiQueue, &nothing, portMAX_DELAY);
      break;
    case 'g':
      xQueueSend(gpsQueue, &nothing, portMAX_DELAY);
      break;
    default:
      ESP_LOGW(TAG, "error=\"unknown serial command\"");
      break;
    }
  }
}
