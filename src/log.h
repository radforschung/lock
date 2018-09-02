#ifndef _lock_log_h
#define _lock_log_h

#include "esp32-hal-log.h"
#include "esp_timer.h"

#define LOG_US_PER_OSTICK_EXPONENT 4
uint32_t log_ticks();

// overriding log format from esp32-hal-log.h
// with our own, https://brandur.org/logfmt compatible log format:
#undef ARDUHAL_LOG_FORMAT
#define ARDUHAL_LOG_FORMAT(letter, format)                                     \
  ARDUHAL_LOG_COLOR_##letter                                                   \
      "time=%d level=" #letter                                                 \
      " file=%s line=%u method=%s " format ARDUHAL_LOG_RESET_COLOR "\r\n",     \
      log_ticks(), pathToFileName(__FILE__), __LINE__, __FUNCTION__

#endif // _lock_log_h
