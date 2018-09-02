#include "globals.h"
#include "serial.h"

void processSerial() {
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
      xQueueSend(taskQueue, &TASK_SEND_LOCATION_WIFI, portMAX_DELAY);
      break;
    default:
      log_w("error=\"unknown serial command\"");
      break;
    }
  }
}
