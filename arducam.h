// Minimal Arducam Mega (SPI) driver on its own SPI port.
// The Heltec LoRa driver owns the global SPI object, so the cameras can't use it
// (the stock Arducam_Mega library is hard-wired to it).
#pragma once
#include <Arduino.h>
#include <SPI.h>

class ArducamMega {
 public:
  ArducamMega(SPIClass &spi, int cs) : spi_(spi), cs_(cs) {}

  // Returns the sensor ID (0x81..0x87), or 0 if nothing answers.
  uint8_t begin() {
    pinMode(cs_, OUTPUT);
    digitalWrite(cs_, HIGH);
    write(0x07, 0x40);                 // reset
    delay(10);
    if (!waitIdle(1000)) return 0;
    id_ = read(0x40);
    if (id_ < 0x81 || id_ > 0x87) return id_ = 0;
    waitIdle(100);
    write(0x0A, 0x78);
    waitIdle(100);
    return id_;
  }

  bool ok() const { return id_ != 0; }

  // Captures a JPEG. Returns its length (0 on failure). jpegOk is true if it starts with FF D8.
  uint32_t capture(const char *res, bool &jpegOk, uint32_t timeoutMs = 5000) {
    jpegOk = false;
    if (!id_) return 0;
    uint8_t code = resCode(res);
    int shots = 1;
    if (code != curRes_) {
      write(0x20, 0x01);               // JPEG
      waitIdle(100);
      write(0x21, code);
      waitIdle(100);
      curRes_ = code;
      shots = 2;                       // first frame after a size change can be stale
    }
    uint32_t len = 0;
    for (int i = 0; i < shots; i++) {
      write(0x04, 0x01);               // clear FIFO flag
      write(0x04, 0x02);               // start capture
      uint32_t t0 = millis();
      while (!(read(0x44) & 0x04)) {
        if (millis() - t0 > timeoutMs) return 0;
        delay(2);
      }
      len = read(0x45) | (read(0x46) << 8) | ((uint32_t)read(0x47) << 16);
    }
    if (len >= 2) {
      uint8_t h[2];
      burstRead(h, 2);
      jpegOk = h[0] == 0xFF && h[1] == 0xD8;
    }
    return len;
  }

 private:
  SPIClass &spi_;
  int cs_;
  uint8_t id_ = 0, curRes_ = 0xFF;

  uint8_t resCode(const char *r) {
    bool legacy = id_ <= 0x84;
    struct { const char *n; uint8_t now, old; } t[] = {
      {"QVGA", 0x03, 0x01}, {"VGA", 0x05, 0x02}, {"SVGA", 0x06, 0x02}, {"HD", 0x08, 0x04},
      {"UXGA", 0x0A, 0x06}, {"FHD", 0x0B, 0x07}, {"QXGA", 0x0C, 0x08}, {"5MP", 0x0D, 0x09}};
    for (auto &e : t)
      if (!strcmp(r, e.n)) return legacy ? e.old : e.now;
    return legacy ? 0x02 : 0x05;       // VGA
  }

  void select() { spi_.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0)); digitalWrite(cs_, LOW); }
  void deselect() { digitalWrite(cs_, HIGH); spi_.endTransaction(); }

  void write(uint8_t addr, uint8_t val) {
    select();
    spi_.transfer(addr | 0x80);
    spi_.transfer(val);
    deselect();
  }

  uint8_t read(uint8_t addr) {
    select();
    spi_.transfer(addr & 0x7F);
    spi_.transfer(0);
    uint8_t v = spi_.transfer(0);
    deselect();
    return v;
  }

  bool waitIdle(uint32_t ms) {
    uint32_t t0 = millis();
    while ((read(0x44) & 0x03) != 0x02) {
      if (millis() - t0 > ms) return false;
      delay(2);
    }
    return true;
  }

  void burstRead(uint8_t *buf, size_t n) {
    select();
    spi_.transfer(0x3C);
    spi_.transfer(0);                  // dummy byte on the first burst
    for (size_t i = 0; i < n; i++) buf[i] = spi_.transfer(0);
    deselect();
  }
};
