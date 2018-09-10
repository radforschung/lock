#include "globals.h"

#include "epaper.h"

const epd_pinmap epd_pins = {.cs = 14, .dc = 25, .rst = 18, .busy = 0};

#define COLORED 0
#define UNCOLORED 1

Epd epd;
QRCode qrcode;

void wakeEpaper() {
  if (epd.Init(lut_full_update) != 0) {
    ESP_LOGE(TAG, "error=\"e-paper init failed\"");
    return;
  }
}

void setupEpaper() {
  wakeEpaper();

  epd.ClearFrameMemory(0xFF);
  epd.DisplayFrame();

  epd.SetFrameMemory(LOGO_IMAGE_DATA);
  epd.DisplayFrame();

  delay(1000);
  renderQR("https://radforschung.org");

  epd.Sleep();
}

void renderQR(char *text) {
  unsigned char image[5025];
  Paint paint(image, 200, 200);
  paint.Clear(UNCOLORED);

  const int qrversion = 4;
  uint8_t qrcodeData[qrcode_getBufferSize(qrversion)];
  qrcode_initText(&qrcode, qrcodeData, qrversion, ECC_MEDIUM, text);

  const int LEFT_PADDING = 20;
  const int TOP_PADDING = 10;
  const int SIZE_MODIFIER = 5;

  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        paint.DrawFilledRectangle(
            LEFT_PADDING + x * SIZE_MODIFIER - SIZE_MODIFIER + 1,
            TOP_PADDING + y * SIZE_MODIFIER - SIZE_MODIFIER + 1,
            LEFT_PADDING + x * SIZE_MODIFIER, TOP_PADDING + y * SIZE_MODIFIER,
            COLORED);
      }
    }
  }

  epd.SetFrameMemory(paint.GetImage(), 0, 0, paint.GetWidth(),
                     paint.GetHeight());
  epd.DisplayFrame();
}
