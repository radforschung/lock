/**
 ******************************************************************************
 * @file    src/main.h
 * @brief   Header for main.c module
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _lock_main_h
#define _lock_main_h

/* Includes ------------------------------------------------------------------*/
#include "location.h"
#include "lock.h"
#include "lorawan.h"
#include "serial.h"
#include "task.h"

const unsigned TX_INTERVAL = 60;

Lock lock = Lock();
Location location = Location();

#endif // _lock_main_h
