/*
   Test program for the base receiver Arduino code

   What it should do:

   Send a commando
*/
/*
   Uncomment in need of debugging
   when in use, Arduino send every action to the Serial Com port
   set Com with a baud-rate of 115200
*/
#define DEBUG

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>

#ifdef DEBUG
#include <printf.h>
#endif

RF24 radio(9, 10); //CE, CSN
const byte localAddr = 0; //node 0 is masternode
const uint32_t listeningPipes[5] = {0x3A3A3AA1UL, 0x3A3A3AB1UL, 0x3A3A3AC1UL, 0x3A3A3AD1UL, 0x3A3A3AE1UL};

/* Datapaket standaard.
   datapaketten verzonden binnen dit project zullen dit formaat hanteren om een uniform systeem te vormen
   command      //commando (8bits) gestuctureerd volgens command table
   destAddr     //adres (6x8bits) ontvangen met pakket, zal volgens commando een ontvangend adres worden of een adres waarnaar gezonden word
   dataValue    //variabele (8bits) om binnenkomende/uitgaande data in op te slagen

   command table
   0 = Stop command
   1 = use sensor for own actuator
   2 = send sensor value to other actuator
   3 = receive sensor value for own actuator
*/
struct dataStruct {
  uint32_t destAddr;
  uint16_t dataValue;
  uint8_t command;
} dataIn, dataOut;

volatile byte nRF_Status;
bool send = 0;

int sens_pin = A0; //analog 0
int act_pin = 5; //D5 and D6 are both connected to the Timer0 counter
int interrupt_pin = 2;

void setup() {
  pinMode(sens_pin, INPUT); //if reading a switch with no external pull-up resistor, change it to INPUT_PULLUP
  pinMode(interrupt_pin, INPUT_PULLUP);
  pinMode(act_pin, OUTPUT);

#ifdef DEBUG
  Serial.begin(115200);
  printf_begin();
#endif
  /*
     Initailisation of the nRF24L01 chip
  */
  radio.begin();
  radio.setAddressWidth(4);
  radio.setPALevel(RF24_PA_HIGH);
  radio.openReadingPipe(0,listeningPipes[localAddr]);
  radio.startListening();

#ifdef DEBUG
  //print all settings of nRF24L01
  radio.printDetails();
  printf("data size: %d bytes\n\r", sizeof(dataStruct));
#endif

/*  while(1)
  {
    radio.stopListening();

 // commando 1 naar node 2.0
 // dataValue

  radio.openWritingPipe(listeningPipes[1]);
  dataOut.command = 1;
  dataOut.destAddr = listeningPipes[1];
  dataOut.dataValue = 0;

#ifdef DEBUG
  Serial.println("Commando 1 send to address: ");
  printf("%ld\n\r", listeningPipes[1]);
#endif
  //verzend commando 1
  for (int i = 0; i<1000; i++){
    if (!radio.write(&dataOut, sizeof(dataStruct)))
    {
      Serial.println("transmission failed");
      break;
    }
    delay(20);
  }

  radio.openWritingPipe(listeningPipes[1]);
  dataOut.command = 3;
  dataOut.destAddr = listeningPipes[1];

#ifdef DEBUG
  Serial.println("Commando 3 send to address: ");
  printf("%ld\n\r", listeningPipes[1]);
#endif

  for(uint8_t i =0; i<255; i++){
    dataOut.dataValue = i;
    
    if (!radio.write(&dataOut, sizeof(dataStruct)))
    {
      Serial.println("transmission failed");
      break;
    }
    delay(20);
  }
  
  //  einde programma, stuur stop commando
  
  dataOut.command = 0;
  dataOut.destAddr = 0x00;
  dataOut.dataValue = 0;

#ifdef DEBUG
  Serial.println("Commando 0 send to address 1");
#endif

  radio.openWritingPipe(listeningPipes[1]);
  if (!radio.write(&dataOut, sizeof(dataStruct)))
  {
    Serial.println("transmission failed");
  }

  delay(1000); //wacht 2s
  }*/
}

void loop() {
   radio.stopListening();

/*
 * commando 1 naar node 2.0
 * dataValue
 */
  radio.openWritingPipe(listeningPipes[1]);
  dataOut.command = 1;
  dataOut.destAddr = listeningPipes[1];
  dataOut.dataValue = 0;

#ifdef DEBUG
  Serial.println("Commando 1 send to address: ");
  printf("%ld\n\r", listeningPipes[1]);
#endif
  //verzend commando 1
  for (int i = 0; i<1000; i++){
    if (!radio.write(&dataOut, sizeof(dataStruct)))
    {
      Serial.println("transmission failed");
    }
    delay(20);
  }

  radio.openWritingPipe(listeningPipes[1]);
  dataOut.command = 3;
  dataOut.destAddr = listeningPipes[1];

#ifdef DEBUG
  Serial.println("Commando 3 send to address: ");
  printf("%ld\n\r", listeningPipes[1]);
#endif

  for(uint8_t i =0; i<255; i++){
    dataOut.dataValue = i;
    
    if (!radio.write(&dataOut, sizeof(dataStruct)))
    {
      Serial.println("transmission failed");
    }
    delay(20);
  }
  
  /*
     einde programma, stuur stop commando
  */
  dataOut.command = 0;
  dataOut.destAddr = 0x00;
  dataOut.dataValue = 0;

#ifdef DEBUG
  Serial.println("Commando 0 send to address 1");
#endif

  radio.openWritingPipe(listeningPipes[1]);
  if (!radio.write(&dataOut, sizeof(dataStruct)))
  {
    Serial.println("transmission failed");
  }
/*
#ifdef DEBUG
  Serial.println("einde programma");
#endif
*/
  delay(1000); //wacht 2s
}
