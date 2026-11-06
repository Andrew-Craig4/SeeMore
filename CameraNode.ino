/*
 * SeeMore camera node: pan/tilt + camera control over LoRa (JSON).
 * Board: Heltec WiFi LoRa 32 (V4 R8). Libraries: ArduinoJson (v7), AccelStepper.
 * Pin map: pins.h (Test 1, direct drive).
 */

#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <SPI.h>
#include <ArduinoJson.h>
#include <AccelStepper.h>
#include "arducam.h"
#include "pins.h"

#define RF_FREQUENCY               915000000
#define TX_OUTPUT_POWER            14
#define LORA_BANDWIDTH             0
#define LORA_SPREADING_FACTOR      7
#define LORA_CODINGRATE            1
#define LORA_PREAMBLE_LENGTH       8
#define LORA_SYMBOL_TIMEOUT        0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON       false
#define BUFFER_SIZE                255

// 28BYJ-48 half-steps per motor shaft turn, and CAD v4.1 gearing
const float HALF_STEPS_PER_REV = 64 * 63.68395f;
const float PAN_RATIO  = 184.0f / 16.0f;
const float TILT_RATIO = 48.0f / 16.0f;
const float PAN_LIMIT  = 170;   // degrees, output shaft
const float TILT_LIMIT = 60;
const float ACCEL      = 1500;  // half-steps/s^2
float maxSpeed         = 800;   // half-steps/s

struct Axis {
  AccelStepper m;
  float stepsPerDeg, limit;
  Axis(const int p[4], float ratio, float lim)
    : m(AccelStepper::HALF4WIRE, p[0], p[2], p[1], p[3], false),
      stepsPerDeg(HALF_STEPS_PER_REV * ratio / 360.0f), limit(lim) {}
  float deg() { return m.currentPosition() / stepsPerDeg; }
  bool moveTo(float d) {
    bool clipped = fabsf(d) > limit;
    d = constrain(d, -limit, limit);
    m.enableOutputs();
    m.moveTo(lroundf(d * stepsPerDeg));
    return clipped;
  }
};

Axis pan(PAN_PINS, PAN_RATIO, PAN_LIMIT);
Axis tilt(TILT_PINS, TILT_RATIO, TILT_LIMIT);
bool moving = false;
int moveId = -1;

SPIClass camSPI(HSPI);   // global SPI belongs to the LoRa radio
ArducamMega camTele(camSPI, PIN_CAM_TELE_CS);
ArducamMega camWide(camSPI, PIN_CAM_WIDE_CS);

static RadioEvents_t RadioEvents;
char rxpacket[BUFFER_SIZE + 1];
volatile bool rxReady = false;
bool txBusy = false, rxOn = false;

String txQueue[6];
int qHead = 0, qLen = 0;

void queueTx(const String &s) {
  if (qLen == 6) { qHead = (qHead + 1) % 6; qLen--; }   // drop oldest
  txQueue[(qHead + qLen) % 6] = s;
  qLen++;
}

int limitsMv() { return analogReadMilliVolts(PIN_LIMITS); }

const char *limitsState(int mv) {
  if (mv > LIMIT_OPEN_MV) return "open";
  if (mv < LIMIT_PAN_MV) return "pan";
  return "tilt";
}

void addPose(JsonDocument &d) {
  d["pan"] = roundf(pan.deg() * 10) / 10;
  d["tilt"] = roundf(tilt.deg() * 10) / 10;
  d["busy"] = moving;
}

void send(JsonDocument &d) {
  String s;
  serializeJson(d, s);
  Serial.println("< " + s);
  queueTx(s);
}

void reply(int id, bool ok, const char *err = nullptr) {
  JsonDocument d;
  if (id >= 0) d["id"] = id;
  d["ok"] = ok;
  if (err) d["err"] = err;
  addPose(d);
  send(d);
}

void snap(int id, const char *which, const char *res) {
  bool tele = strcmp(which, "wide") != 0;
  ArducamMega &cam = tele ? camTele : camWide;
  if (!cam.ok()) return reply(id, false, "camera not found");
  if (moving) return reply(id, false, "busy");

  uint32_t t0 = millis();
  bool jpeg;
  uint32_t len = cam.capture(res, jpeg);

  JsonDocument d;
  if (id >= 0) d["id"] = id;
  d["ok"] = len > 2 && jpeg;
  d["cam"] = tele ? "tele" : "wide";
  d["res"] = res;
  d["bytes"] = len;
  d["ms"] = millis() - t0;
  send(d);
}

void handle(const char *msg) {
  Serial.printf("> %s\n", msg);
  JsonDocument in;
  if (deserializeJson(in, msg)) return reply(-1, false, "bad json");

  int id = in["id"] | -1;
  const char *cmd = in["cmd"] | "";

  if (!strcmp(cmd, "ping")) {
    JsonDocument d;
    if (id >= 0) d["id"] = id;
    d["ok"] = true;
    d["ms"] = millis();
    send(d);
  } else if (!strcmp(cmd, "status")) {
    JsonDocument d;
    if (id >= 0) d["id"] = id;
    d["ok"] = true;
    addPose(d);
    int mv = limitsMv();
    d["lim"] = limitsState(mv);
    d["limMv"] = mv;
    d["tele"] = camTele.ok();
    d["wide"] = camWide.ok();
    d["speed"] = maxSpeed;
    send(d);
  } else if (!strcmp(cmd, "move") || !strcmp(cmd, "jog")) {
    bool rel = cmd[0] == 'j';
    bool clipped = false;
    if (in["pan"].is<float>())  clipped |= pan.moveTo((rel ? pan.deg() : 0) + in["pan"].as<float>());
    if (in["tilt"].is<float>()) clipped |= tilt.moveTo((rel ? tilt.deg() : 0) + in["tilt"].as<float>());
    moving = true;
    moveId = id;
    reply(id, true, clipped ? "clipped to soft limit" : nullptr);
  } else if (!strcmp(cmd, "stop")) {
    pan.m.stop();
    tilt.m.stop();
    reply(id, true);
  } else if (!strcmp(cmd, "zero")) {
    if (moving) return reply(id, false, "busy");
    pan.m.setCurrentPosition(0);
    tilt.m.setCurrentPosition(0);
    reply(id, true);
  } else if (!strcmp(cmd, "speed")) {
    maxSpeed = constrain(in["v"] | maxSpeed, 100.0f, 1000.0f);
    pan.m.setMaxSpeed(maxSpeed);
    tilt.m.setMaxSpeed(maxSpeed);
    reply(id, true);
  } else if (!strcmp(cmd, "release")) {
    pan.m.disableOutputs();
    tilt.m.disableOutputs();
    reply(id, true);
  } else if (!strcmp(cmd, "snap")) {
    snap(id, in["cam"] | "tele", in["res"] | "VGA");
  } else {
    reply(id, false, "unknown cmd");
  }
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  size = min<uint16_t>(size, BUFFER_SIZE);
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';
  rxReady = true;
  rxOn = false;
}

void OnTxDone(void) { txBusy = false; }
void OnTxTimeout(void) { Radio.Sleep(); txBusy = false; }
void OnRxTimeout(void) { rxOn = false; }
void OnRxError(void) { rxOn = false; }

void setup() {
  for (int p : PAN_PINS)  { pinMode(p, OUTPUT); digitalWrite(p, LOW); }
  for (int p : TILT_PINS) { pinMode(p, OUTPUT); digitalWrite(p, LOW); }
  pinMode(PIN_CAM_TELE_CS, OUTPUT); digitalWrite(PIN_CAM_TELE_CS, HIGH);
  pinMode(PIN_CAM_WIDE_CS, OUTPUT); digitalWrite(PIN_CAM_WIDE_CS, HIGH);

  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  pinMode(PIN_VEXT, OUTPUT);
  digitalWrite(PIN_VEXT, VEXT_ON);
  delay(100);

  analogSetPinAttenuation(PIN_LIMITS, ADC_11db);

  for (Axis *a : {&pan, &tilt}) {
    a->m.setMaxSpeed(maxSpeed);
    a->m.setAcceleration(ACCEL);
  }

  camSPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
  uint8_t tid = camTele.begin(), wid = camWide.begin();
  Serial.printf("tele camera 0x%02X, wide camera 0x%02X (0 = not found)\n", tid, wid);

  RadioEvents.RxDone = OnRxDone;
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  JsonDocument d;
  d["evt"] = "boot";
  d["tele"] = camTele.ok();
  d["wide"] = camWide.ok();
  send(d);
}

void loop() {
  Radio.IrqProcess();

  if (rxReady) {
    rxReady = false;
    handle(rxpacket);
  }

  pan.m.run();
  tilt.m.run();

  if (moving && pan.m.distanceToGo() == 0 && tilt.m.distanceToGo() == 0) {
    moving = false;
    pan.m.disableOutputs();
    tilt.m.disableOutputs();
    JsonDocument d;
    d["evt"] = "done";
    if (moveId >= 0) d["id"] = moveId;
    addPose(d);
    d["lim"] = limitsState(limitsMv());
    send(d);
  }

  if (!txBusy && qLen > 0) {
    String &s = txQueue[qHead];
    qHead = (qHead + 1) % 6;
    qLen--;
    txBusy = true;
    rxOn = false;
    Radio.Send((uint8_t *)s.c_str(), s.length());
  } else if (!txBusy && !rxOn) {
    rxOn = true;
    Radio.Rx(0);
  }
}
