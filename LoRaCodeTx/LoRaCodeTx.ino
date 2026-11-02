/* 
* 
* Project Name: Wifi LoRa Transmitter
* Author: Isabella Vargas
* 
*/

#include "LoRaWan_APP.h"
#include "Arduino.h"


#define RF_FREQUENCY                                915000000 // Hz

#define TX_OUTPUT_POWER                             5        // dBm

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 255 //payload size

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

double txNumber;

int timeStart;
int timeEnd;

bool lora_idle=true;

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );

void SendLargePacket(char packet[], int size);

//about a thousand characters
char largePacket[] = "When he was nearly thirteen, my brother Jem got his arm badly broken at the elbow. "
"When it healed, and Jem's fears of never being able to play football were assuaged, he "
"was seldom self-conscious about his injury. His left arm was somewhat shorter than his "
"right; when he stood or walked, the back of his hand was at right angles to his body, his "
"thumb parallel to his thigh. He couldn't have cared less, so long as he could pass and "
"punt."
" When enough years had gone by to enable us to look back on them, we sometimes "
"discussed the events leading to his accident. I maintain that the Ewells started it all, but "
"Jem, who was four years my senior, said it started long before that. He said it began the "
"summer Dill came to us, when Dill first gave us the idea of making Boo Radley come "
"out."
" I said if he wanted to take a broad view of the thing, it really began with Andrew "
"Jackson. If General Jackson hadn't run the Creeks up the creek, Simon Finch would "
"never have paddled up the Alabama, and where would we be if he hadn't? We were far "
"too old to settle an argument with a fist-fight, so we consulted Atticus. Our father said we "
"were both right.";

void setup() 
{
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
    
  Radio.Init( &RadioEvents );
  Radio.SetChannel( RF_FREQUENCY );
  Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                 LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                 LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                 true, 0, 0, LORA_IQ_INVERSION_ON, 3000 ); 
}

void loop()
{
	if(lora_idle == true)
	{
    delay(1000);
    timeStart = millis(); // keeping track of how long it takes to send a packet
    SendLargePacket(largePacket, strlen(largePacket));
    timeEnd = millis();
    Serial.printf("\r\nFull packet of length %d sent in %d milliseconds\r\n",strlen(largePacket),(timeEnd - timeStart));
	}
  Radio.IrqProcess( );
}


void OnTxDone(void)
{
	Serial.println("TX done....");
	lora_idle = true;
}

void OnTxTimeout(void)
{
  Radio.Sleep( );
  Serial.println("TX Timeout....");
  lora_idle = true;
}


void SendLargePacket(char packet[], int size)
{
  int snipBegin = 0;
  int count = 1;
  Serial.println("Sending out larget packet...");
  while(snipBegin < size)
  {
    if (lora_idle == true)
    {
      delay(1000);

      if ((size - snipBegin) > 250)
      {
        memcpy(txpacket, packet + snipBegin, 250 * sizeof(char));
      }
      else
      {
        memcpy(txpacket, packet + snipBegin, (size - snipBegin) * sizeof(char));
      }
      
      Radio.Send( (uint8_t *)txpacket, strlen(txpacket) ); //send the package out	

      lora_idle = false;
      snipBegin += 250;
      Serial.printf("\r\nPacket %d sent...\r\n",count);
      count++;
    }
    Radio.IrqProcess( );
  }
  Serial.println("Finished sending out packet");
}