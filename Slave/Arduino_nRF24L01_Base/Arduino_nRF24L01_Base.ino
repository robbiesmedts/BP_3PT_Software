/* About the software
 *  
 * Start-up for the Arduino Sensor/Actuator side of the project
 * What it should do:
 * receive commando's adressed to it's address,
 * depending on the commando one of the following actions:
 * 
 * 0) do nothing / stop the current action
 * 1) read sensor value and use it for own actuator
 * 2) read sensor value and send to received address
 * 3) read received data and apply to actuator
 * 4) reset the node / recalibrate sensor & actuator
 * 
 * flow altering defined variables:
 * DEBUG
 *      enables all the debugging functionality and sends informational data over Serial COM port to an connected PC (baud 115200)
 * CONTINIOUS
 *      enables the node to to excecute a given command until an other command is received.
 *      if CONTINIOUS is not active a command is excecuted once after the command is received.
*/
#define DEBUG
//#define CONTINIOUS

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#ifdef DEBUG
#include <printf.h>
#endif

RF24 radio(9, 10); //CE, CSN
const byte localAddr = 1; //node x in systeem // node 0 is masternode
const uint32_t listeningPipes[5] = {0x3A3A3AA1UL, 0x3A3A3AB1UL, 0x3A3A3AC1UL, 0x3A3A3AD1UL, 0x3A3A3AE1UL}; 
bool b_tx_ok, b_tx_fail, b_rx_ready = 0;

/* Datapaket standaard.
   datapaketten verzonden binnen dit project zullen dit formaat hanteren om een uniform systeem te vormen
   destAddr     //adres (6x8bits) ontvangen met pakket, zal volgens commando een ontvangend adres worden of een adres waarnaar gezonden word
   dataValue    //variabele (8bits) om binnenkomende/uitgaande data in op te slagen
   command      //commando (8bits) gestuctureerd volgens command table

   command table
   0 = Stop command
   1 = use sensor for own actuator
   2 = send sensor value to other actuator
   3 = receive sensor value for own actuator
   4 = reset node
*/
struct dataStruct {
  uint32_t destAddr;
  uint16_t dataValue;
  uint8_t command;
} dataIn, dataOut;

int sens_pin = A1; //analog 0
int act_pin = 5; //D5 and D6 are both connected to the Timer0 counter
const int interrupt_pin = 2;

void setup() {
  
  pinMode(sens_pin, INPUT); //if reading a switch with no external pull-up resistor, change it to INPUT_PULLUP
  pinMode(interrupt_pin, INPUT_PULLUP);
  pinMode(act_pin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(interrupt_pin), nRF_IRQ, LOW);

#ifdef DEBUG
  Serial.begin(115200);
  printf_begin();
#endif
  //Initailisation of the nRF24L01 chip
  radio.begin();
  radio.setAddressWidth(4);
  for (uint8_t i = 0; i < 4; i++)
  radio.openReadingPipe(i, listeningPipes[localAddr] + i);
  
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();

#ifdef DEBUG
  //print all settings of nRF24L01
  radio.printDetails();
#endif
}

void loop() {
  uint8_t currentCommand;
  uint16_t currentValue;
  uint32_t currentDestAddr;
  
  /* Uitvoering op interrupt basis
   * commando wordt opgeslagen
   * en uitgevoerd tot een ander commando verzonden wordt 
  */
#ifdef CONTINIOUS
  if(b_rx_ready){
    b_rx_ready = 0;
    radio.read(&dataIn, sizeof(dataIn));

    currentCommand = dataIn.command;
    currentValue = dataIn.dataValue;
    currentDestAddr = dataIn.destAddr;
#ifdef DEBUG
      Serial.println("IRQ geweest");
      printf("Current command: %d\n\r", currentCommand);
#endif
  }//end fetch command
  
  switch (currentCommand){
    case 0:
      //stop command, hold last value
      //analogWrite(act_pin, 0);
      break;

    case 1: //read sensor and use fo own actuator
      analogWrite(act_pin, analogRead(sens_pin));
      break;

    case 2: // read sensor and send to other actuator
      dataOut.command = 3;
      dataOut.dataValue = analogRead(sens_pin);
      dataOut.destAddr = listeningPipes[localAddr];
        
      radio.stopListening();
      radio.openWritingPipe(dataIn.destAddr);//set destination address
      radio.write(&dataOut, sizeof(dataOut));
#ifdef DEBUG
      printf("%ld", dataIn.destAddr);
      Serial.print("\n\rdata send: ");
      Serial.println(dataOut.dataValue);
#endif
      //radio.openReadingPipe(0,listeningPipes[localAddr]);
      radio.startListening();
      break;

    case 3: //receive sensor value and use for own actuator
#ifdef DEBUG
      Serial.print("receiving address: ");
      printf("%ld", dataIn.destAddr);
      Serial.println();
      Serial.print("received data: ");
      Serial.println(dataIn.dataValue);
#endif
      analogWrite(act_pin, dataIn.dataValue);
      break;
    case 4:
#ifdef DEBUG
      Serial.print("reset node");
#endif
      //reset node
    default:
      //do nothing
      break;
    /* delay om uitvoering te vertragen
     * uitvoering wordt vertraagd met X ms 
     * precieze berekening is onbekend, maar 1/x is close enough
     */
    delay(4);
  }// end switch
#endif

/* 
 *  Verloop uitvoering als er commando binnen komt.
 *  Verloopt op interruptbasis om processing te verlagen
*/
#ifndef CONTINIOUS
  if(b_rx_ready){
    b_rx_ready = 0; 
    radio.read(&dataIn, sizeof(dataIn));
#ifdef DEBUG
    Serial.print("Incomming command: ");
    Serial.println(dataIn.command);
#endif

    switch (dataIn.command){
      case 0:
        //stop command, do nothing
        //analogWrite(act_pin, 0);
        break;

      case 1: //read sensor and use fo own actuator
        //analogWrite(act_pin, analogRead(sens_pin));
        break;

      case 2: // read sensor and send to other actuator
        dataOut.command = 3;
        dataOut.dataValue = analogRead(sens_pin);
        dataOut.destAddr = listeningPipes[localAddr];
        
        radio.stopListening();
        radio.openWritingPipe(dataIn.destAddr);//set destination address
        if(!radio.write(&dataOut, sizeof(dataOut)))
        {
          Serial.println("radio.write error");
        }
        //delayMicroseconds(50);
        radio.startListening();
#ifdef DEBUG
        Serial.print(dataIn.destAddr, HEX);
        Serial.print("\n\rdata send: ");
        Serial.println(dataOut.dataValue);
#endif
        break;

      case 3: //receive sensor value and use for own actuator
#ifdef DEBUG
        Serial.print("receiving address: ");
        Serial.print(dataIn.destAddr, HEX);
        Serial.println();
        Serial.print("received data: ");
        Serial.println(dataIn.dataValue);
#endif
        //analogWrite(act_pin, dataIn.dataValue);
        break;
      case 4:
#ifdef DEBUG
        Serial.println("reset node");
#endif
        //reset node
      default:
        //do nothing
        break;
    }//end switch
  }//end non-interrupt
#endif  
} //end loop

void nRF_IRQ() {
  noInterrupts();
  radio.whatHappened(b_tx_ok, b_tx_fail, b_rx_ready);
  interrupts();
}
