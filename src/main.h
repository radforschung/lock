#ifndef _lock_main_h
#define _lock_main_h

#include <Preferences.h>
#include "lock.h"
#include "location.h"
#include "lorawan.h"

static const char *TAG = "main";
const unsigned TX_INTERVAL = 60;

Preferences preferences;
Lock lock = Lock();
Location location = Location();


#endif // _lock_main_h
