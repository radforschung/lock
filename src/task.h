#ifndef _lock_task_h
#define _lock_task_h

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

extern QueueHandle_t taskQueue;
static const int TASK_UNLOCK = 1;
static const int TASK_RESTART = 2;
static const int TASK_SEND_LOCK_STATUS = 3;
static const int TASK_SEND_LOCATION_WIFI = 4;

#endif // _lock_task_h
