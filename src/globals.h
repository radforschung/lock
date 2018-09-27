#ifndef _lock_globals_h
#define _lock_globals_h

// overriding with our own, https://brandur.org/logfmt compatible log format:
#ifdef ARDUHAL_LOG_FORMAT
#undef ARDUHAL_LOG_FORMAT
#define ARDUHAL_LOG_FORMAT(letter, format)                                     \
  ARDUHAL_LOG_COLOR_##letter                                                   \
      "time=%d level=" #letter                                                 \
      " file=%s line=%u method=%s " format ARDUHAL_LOG_RESET_COLOR "\r\n",     \
      hal_ticks(), pathToFileName(__FILE__), __LINE__, __FUNCTION__
#endif

// TODO this should be refactored to lock.h
// latch switch: (PULLUP!) LOW: closed, HIGH: open
#define PIN_LOCK_LATCH_SWITCH GPIO_NUM_4

#include "lorawan.h"

extern QueueHandle_t taskQueue;
static const int TASK_UNLOCK = 1;
static const int TASK_RESTART = 2;
static const int TASK_SEND_LOCK_STATUS = 3;
static const int TASK_SEND_LOCATION_WIFI = 4;

#endif // _lock_globals_h
