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

#include "lorawan.h"

extern QueueHandle_t wifiQueue;
extern QueueHandle_t gpsQueue;
extern QueueHandle_t epaperQueue;
extern QueueHandle_t taskQueue;
static const int TASK_UNLOCK = 1;
static const int TASK_RESTART = 2;
static const int TASK_SEND_LOCK_STATUS = 3;
static const int TASK_SEND_LOCATION_WIFI = 4;
static const int TASK_SEND_LOCATION_GPS = 5;

// FIXME: move!
static const int EPAPER_SCREEN_EMPTY = 1;
static const int EPAPER_SCREEN_LOGO = 2;
static const int EPAPER_SCREEN_QR = 3;

static const int LOCK_TASK_UNLOCK = 1;
static const int LOCK_TASK_PARK = 2;
static const int LOCK_TASK_SEND_STATUS = 3;
static const int LOCK_TASK_GPIO_ISR = 4;

#endif // _lock_globals_h
