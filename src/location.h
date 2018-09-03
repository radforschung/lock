/**
  ******************************************************************************
  * @file    src/location.h
  * @brief   Header for location.c module
  ******************************************************************************
  */

/* Define to prevent recursive inclusion ------------------------------------ation*/
#ifndef _lock_location_h
#define _lock_location_h

/* Includes ------------------------------------------------------------------*/
#include <vector>
#include "WiFi.h"


/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
// wifis which contain this string did
// opt out from location services
static const String nomapSuffix = "_nomap";
static const int maxScanWifis = 7;

static const int GPSRX = 15;
static const int GPSTX = 12;
static const uint32_t GPSBaud = 4800;


/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
class Location {
public:
  Location();
  std::vector<uint8_t> scanWifis();
};

void gps_task(void *ignore);

#endif // _lock_location_h
