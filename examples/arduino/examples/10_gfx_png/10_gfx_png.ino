
#define PNG_FILENAME "/images/octocat.png"
#define PNG_4BPP_FILENAME "/images/octocat-4bpp.png"

#include <Arduino_GFX_Library.h>
#include "TCA9554.h"
#include <FFat.h>
#include <SD_MMC.h>
#include <PNGdec.h>

#define GFX_BL 6  // default backlight pin, you may replace DF_GFX_BL to actual backlight pin

#define SPI_MISO 2
#define SPI_MOSI 1
#define SPI_SCLK 5

#define LCD_CS -1
#define LCD_DC 3
#define LCD_RST -1
#define LCD_HOR_RES 320
#define LCD_VER_RES 480

#define I2C_SDA 8
#define I2C_SCL 7

int clk = 11;
int cmd = 10;
int d0 = 9;

TCA9554 TCA(0x20);

Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC /* DC */, LCD_CS /* CS */, SPI_SCLK /* SCK */, SPI_MOSI /* MOSI */, SPI_MISO /* MISO */);
Arduino_GFX *gfx = new Arduino_ST7796(
  bus, LCD_RST /* RST */, 0 /* rotation */, true, LCD_HOR_RES, LCD_VER_RES);


int16_t w, h, xOffset, yOffset;

PNG png;
// Functions to access a file on the SD card
File pngFile;

void *myOpen(const char *filename, int32_t *size) {
  pngFile = SD_MMC.open(filename, "r");

  if (!pngFile || pngFile.isDirectory()) {
    Serial.println(F("ERROR: Failed to open " PNG_FILENAME " file for reading"));
    gfx->println(F("ERROR: Failed to open " PNG_FILENAME " file for reading"));
  } else {
    *size = pngFile.size();
    Serial.printf("Opened '%s', size: %d\n", filename, *size);
  }

  return &pngFile;
}

void myClose(void *handle) {
  if (pngFile)
    pngFile.close();
}

int32_t myRead(PNGFILE *handle, uint8_t *buffer, int32_t length) {
  if (!pngFile)
    return 0;
  return pngFile.read(buffer, length);
}

int32_t mySeek(PNGFILE *handle, int32_t position) {
  if (!pngFile)
    return 0;
  return pngFile.seek(position);
}

// Function to draw pixels to the display
void PNGDraw(PNGDRAW *pDraw) {
  uint16_t usPixels[320];
  uint8_t usMask[480];

  // Serial.printf("Draw pos = 0,%d. size = %d x 1\n", pDraw->y, pDraw->iWidth);
  png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0x00000000);
  png.getAlphaMask(pDraw, usMask, 1);
  gfx->draw16bitRGBBitmapWithMask(xOffset, yOffset + pDraw->y, usPixels, usMask, pDraw->iWidth, 1);
}

void lcd_reset(void) {
  TCA.write1(1, 1);
  delay(10);
  TCA.write1(1, 0);
  delay(10);
  TCA.write1(1, 1);
  delay(200);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Arduino_GFX JPEG Image Viewer example");
  Wire.begin(I2C_SDA, I2C_SCL);
  TCA.begin();
  TCA.pinMode1(1,OUTPUT);
  lcd_reset();
  // Init Display
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(RGB565_BLACK);

  w = gfx->width(), h = gfx->height();
#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif
  if (!SD_MMC.setPins(clk, cmd, d0)) {
    Serial.println("Pin change failed!");
    return;
  }
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println(F("ERROR: File System Mount Failed!"));
    gfx->println(F("ERROR: File System Mount Failed!"));
  } else {
    unsigned long start = millis();
    int rc;
    rc = png.open(PNG_FILENAME, myOpen, myClose, myRead, mySeek, PNGDraw);
    if (rc == PNG_SUCCESS) {
      int16_t pw = png.getWidth();
      int16_t ph = png.getHeight();

      xOffset = (w - pw) / 2;
      yOffset = (h - ph) / 2;

      rc = png.decode(NULL, 0);

      Serial.printf("Draw offset: (%d, %d), time used: %lu\n", xOffset, yOffset, millis() - start);
      Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
      png.close();
    } else {
      Serial.println("png.open() failed!");
    }
  }

  delay(5000);  // 5 seconds
}

void loop() {
  unsigned long start = millis();
  int rc;
  rc = png.open(PNG_4BPP_FILENAME, myOpen, myClose, myRead, mySeek, PNGDraw);
  if (rc == PNG_SUCCESS) {
    // random draw position
    int16_t pw = png.getWidth();
    int16_t ph = png.getHeight();
    xOffset = random(w) - (pw / 2);
    yOffset = random(h) - (ph / 2);

    rc = png.decode(NULL, 0);

    Serial.printf("Draw offset: (%d, %d), time used: %lu\n", xOffset, yOffset, millis() - start);
    Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
    png.close();
  } else {
    Serial.println("png.open() failed!");
  }

  delay(1000);  // 1 second
}
