/*
 * SeeMore base station: Serial Monitor <-> LoRa bridge for JSON commands.
 * Type a JSON line (e.g. {"id":1,"cmd":"move","pan":30}) and press Enter.
 * Replies from the camera node print with their RSSI.
 * Board: any Heltec WiFi LoRa 32 (V3/V4). Serial Monitor: 115200, line ending "Newline".
 */

#include "LoRaWan_APP.h"
#include "Arduino.h"

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

static RadioEvents_t RadioEvents;
char rxpacket[BUFFER_SIZE + 1];
char line[BUFFER_SIZE + 1];
int lineLen = 0;
bool txBusy = false, rxOn = false;

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  size = min<uint16_t>(size, BUFFER_SIZE);
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';
  Serial.printf("< %s  (rssi %d, snr %d)\n", rxpacket, rssi, snr);
  rxOn = false;
}

void OnTxDone(void) { txBusy = false; }
void OnTxTimeout(void) { Radio.Sleep(); txBusy = false; Serial.println("! tx timeout"); }
void OnRxTimeout(void) { rxOn = false; }
void OnRxError(void) { rxOn = false; }

void setup() {
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

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
  Serial.println("SeeMore base station ready. Type a JSON command.");
}

void loop() {
  Radio.IrqProcess();

  while (Serial.available() && !txBusy) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c != '\n') {
      if (lineLen < BUFFER_SIZE) line[lineLen++] = c;
      continue;
    }
    line[lineLen] = '\0';
    if (lineLen > 0) {
      Serial.printf("> %s\n", line);
      txBusy = true;
      rxOn = false;
      Radio.Send((uint8_t *)line, lineLen);
    }
    lineLen = 0;
  }

  if (!txBusy && !rxOn) {
    rxOn = true;
    Radio.Rx(0);
  }
}
