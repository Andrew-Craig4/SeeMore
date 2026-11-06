// SeeMore camera unit: Test 1 GPIO map, rev D
// Heltec WiFi LoRa 32 V4-R8 (ESP32-S3R8), No Display version.
// Test 1 = pan + tilt motors wired straight to their ULN2003 boards, tele + wide Arducam Mega,
// two limit switches. Uses every usable header pin.
// Source of truth for the sketch. Change pins here and on the pin map page together.
#pragma once
#include <Arduino.h>

// Pan and tilt ULN2003 inputs, in IN1..IN4 order.
// Set all eight as outputs and LOW first thing in setup(): GPIO43 flickers during boot.
constexpr int PAN_PINS[4]  = {3, 4, 6, 21};
constexpr int TILT_PINS[4] = {41, 43, 44, 45};   // 45 is a strapping pin: never add a pull-up to it
// AccelStepper HALF4WIRE wants IN1, IN3, IN2, IN4:
//   AccelStepper pan (AccelStepper::HALF4WIRE, 3, 6, 4, 21);
//   AccelStepper tilt(AccelStepper::HALF4WIRE, 41, 44, 43, 45);

// Shared SPI bus for both Arducam Megas
constexpr int PIN_SPI_SCK     = 47;
constexpr int PIN_SPI_MOSI    = 48;
constexpr int PIN_SPI_MISO    = 38;
constexpr int PIN_CAM_TELE_CS = 39;
constexpr int PIN_CAM_WIDE_CS = 42;   // GNSS power-control line on this board; idles high, which suits a CS

// Limit switches: one analog pin, resistor ladder.
// 10k pull-up to Ve. Pan switch shorts to GND. Tilt switch shorts to GND through 10k.
constexpr int PIN_LIMITS    = 1;      // read with analogReadMilliVolts()
constexpr int LIMIT_OPEN_MV = 2300;   // above this: nothing pressed (expect ~3000 mV)
constexpr int LIMIT_PAN_MV  = 800;    // below this: pan pressed (expect ~0 mV). Between: tilt (~1600 mV).

// On the board
constexpr int PIN_VEXT      = 40;     // switches the Ve pins. Check polarity on the bench, then set VEXT_ON.
constexpr int VEXT_ON       = HIGH;   // R8 datasheet says HIGH = on. Older Heltec boards use LOW.
constexpr int PIN_LED       = 46;     // white user LED
constexpr int PIN_PRG       = 0;      // PRG button (strapping pin, input only)

// LoRa SX1262 (wired inside the board, listed so nothing else claims them)
constexpr int PIN_LORA_NSS  = 8;
constexpr int PIN_LORA_SCK  = 9;
constexpr int PIN_LORA_MOSI = 10;
constexpr int PIN_LORA_MISO = 11;
constexpr int PIN_LORA_RST  = 12;
constexpr int PIN_LORA_BUSY = 13;
constexpr int PIN_LORA_DIO1 = 14;
constexpr int PIN_LORA_FEM_EN  = 2;
constexpr int PIN_LORA_PA_CTX  = 5;
constexpr int PIN_LORA_FEM_PWR = 7;
