#ifndef _lock_location_h
#define _lock_location_h

#include <vector>
#include "WiFi.h"

class Location {
public:
  Location();
  std::vector<uint8_t> scanWifis();
};

#endif // _lock_location_h
