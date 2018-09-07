#include "globals.h"

#include "epaper.h"

const epd_pinmap epd_pins = {.cs = 14, .dc = 25, .rst = 12, .busy = 0};

Epd epd;

void setupEpaper() {
  if (epd.Init(lut_full_update) != 0) {
    ESP_LOGE(TAG, "error=\"e-paper init failed\"");
    return;
  }

  epd.ClearFrameMemory(0xFF);
  epd.DisplayFrame();

  epd.SetFrameMemory(LOGO_IMAGE_DATA);
  epd.DisplayFrame();

  epd.Sleep();
}
