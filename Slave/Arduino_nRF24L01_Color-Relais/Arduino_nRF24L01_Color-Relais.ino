/**About the software*
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
 * When the sensor is used, the received value represents the compared color value
 * 
 **About the hardware*
 *
 * Software to be build for the Arduino Nano
 * input and output modules are:
 * - Velleman VMA325 color sensor
 * - SRD-05VDC relais module
 * 
 **How to connect the prototype*
 * VMA325   ||    Arduino
 * S0       ||    D4
 * S1       ||    D5
 * S2       ||    D6
 * S3       ||    D7
 * OUT      ||    D8
 * OE       ||    A1
 * LED      ||    A0
 * GND/G    ||    GND
 * V        ||    5V
 * 
 * Relais   ||    Arduino
 * S        ||    A5
 * +        ||    5V
 * -        ||    GND
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

/* Measured values from color calibration
 * LOW values are measured by high intensities
 * HIGH values are measured by low intensities
 * 
 * the reverse values come from the function used to measure the output frequency.
 * the requency is meqsured qnd returned in microseconds...
 * So a high frequency results in a low value in microseconds
 * 
 * Intensity clip: RGB_LOW values = 3
 * Complete darkness: RGB_HIGH > 100000
 * on-board LED: 80 < RGB_LOW < 105
 * 120 < RGB_HIGH < 160
 */
#define R_LOW 3
#define R_HIGH 100000
#define G_LOW 3
#define G_HIGH 100000
#define B_LOW 3
#define B_HIGH 100000

RF24 radio(9, 10); //CE, CSN
const byte localAddr = 1; //node x in systeem // node 0 is masternode
const uint32_t listeningPipes[5] = {0x3A3A3AA1UL, 0x3A3A3AB1UL, 0x3A3A3AC1UL, 0x3A3A3AD1UL, 0x3A3A3AE1UL}; 
bool b_tx_ok, b_tx_fail, b_rx_ready = 0;
uint8_t RGB[3] = {0, 0, 0};

/* Datapaket standaard.
   datapaketten verzonden binnen dit project zullen dit formaat hanteren om een uniform systeem te vormen
   destAddr     //adres (6x8bits) ontvangen met pakket, zal volgens commando een ontvangend adres worden of een adres waarnaar gezonden word
   dataValue    //variabele (16bits) om binnenkomende/uitgaande data in op te slagen
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

/* List with determent colours
 * Calibrated with LEE colour gels
 */
typedef enum {
  BLACK = 1,
  RED = 10,      //L106 (Primary Red)
  ORANGE = 20,   //L105 (Orange)
  YELLOW = 30,   //L101 (Yellow)
  GREEN = 40,    //L139 (Primary Green)
  CYAN = 50,     //L118 (Light Blue)
  BLUE = 60,     //L132 (Medium Blue)
  INDIGO = 70,   //L071 (Tokyo Blue)
  PURPLE = 80,   //L049 (Medium Purple)
  MAGENTA = 90   //L113 (Magenta)
}TSCcolor_e;

/* Pin definitions */
const int interrupt_pin = 2; //nRF24L01 interrupt

const int C_OUT = 3; //TCS230 color value interrupt
const int S0 = 4; //output scaling 1
const int S1 = 5; //output scaling 2
const int S2 = 6; //photodiode selection 1
const int S3 = 7; //photodiode selection 2
const int LED = A0;
const int OE = A1;

const int RELAIS = A5;

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
  printf_begin();
#endif

  /*
   * Initialisation of the color sensor
   */
  TCS_Init();

  pinMode(RELAIS, OUTPUT);
  
  //nRf24L01 pin configurations
  pinMode(interrupt_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interrupt_pin), nRF_IRQ, LOW); //INT0
  
  /*
     Initailisation of the nRF24L01 chip
  */
  radio.begin();
  radio.setAddressWidth(4);
  for (uint8_t i = 0; i < 4; i++)
    radio.openReadingPipe(0, listeningPipes[localAddr] + i);
    
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
  uint8_t color;
  
  /* Uitvoering op interrupt basis
   * commando wordt opgeslagen
   * en uitgevoerd tot een ander commando verzonden wordt 
  */
#ifdef CONTINIOUS
/*  if(b_rx_ready){
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
    // delay om uitvoering te vertragen
    // uitvoering wordt vertraagd met X ms 
    // precieze berekening is onbekend, maar 1/x is close enough
    //
    delay(4);
  }// end switch
*/
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
        digitalWrite(RELAIS, LOW);
        break;

      case 1: //read sensor and use fo own actuator
        TCS_read(RGB);
        color = TSC_Color(RGB);
        if (color == dataIn.dataValue){
          digitalWrite(RELAIS, HIGH);
        }
        else{
          digitalWrite(RELAIS, LOW);
        }
        break;

      case 2: // read sensor and send to other actuator
        dataOut.command = 3;
        for (uint8_t i = 0; i < 3; i++)
        {
          dataOut.dataValue = RGB[i];
          dataOut.destAddr = listeningPipes[localAddr]+i;

          radio.stopListening();
          radio.openWritingPipe(dataIn.destAddr+i);//set destination address
          radio.write(&dataOut, sizeof(dataOut));
        }
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
        digitalWrite(RELAIS, dataIn.dataValue);
        break;
      case 4:
#ifdef DEBUG
        Serial.print("reset node");
#endif
        //reset node
      default:
        //do nothing
        break;
    }//end switch
  }//end non-interrupt
#endif  
} //end loop

//init TSC230 and setting frequency.
void TCS_Init(){
  //output frequency scaling selection ports
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  //Photo diode selection ports
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  //output frequency
  pinMode(C_OUT, INPUT);
  //White LEDs, active LOW
  pinMode(LED, OUTPUT);
  //Output Enable, active low
  pinMode(OE, OUTPUT);

  digitalWrite(S0, HIGH);// OUTPUT FREQUENCY SCALING 20%
  digitalWrite(S1, LOW);
  digitalWrite(LED, HIGH); // LOW = Switch ON the 4 LED's , HIGH = switch off the 4 LED's
  digitalWrite(OE, LOW);
}

//read the colordata from the TSC230
void TCS_read(uint8_t* RGB){
  uint32_t redFrequency, greenFrequency, blueFrequency;
  
  //enable the output of the sensor
  digitalWrite(OE, LOW);
  
  // Setting RED (R) filtered photodiodes to be read
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);

  // Reading the output frequency
  redFrequency = pulseIn(C_OUT, LOW);
  // Remaping the value of the RED (R) frequency from 0 to 255
  RGB[0] = map(redFrequency, R_LOW, R_HIGH, 255,0);
  delay(5);
  
  // Setting GREEN (G) filtered photodiodes to be read
  digitalWrite(S2, HIGH);
  digitalWrite(S3, HIGH);

  // Reading the output frequency
  greenFrequency = pulseIn(C_OUT, LOW);
  // Remaping the value of the GREEN (G) frequency from 0 to 255
  RGB[1] = map(redFrequency, G_LOW, G_HIGH, 255,0);
  delay(5);
  
  // Setting BLUE (B) filtered photodiodes to be read
  digitalWrite(S2, LOW);
  digitalWrite(S3, HIGH);

  // Reading the output frequency
  blueFrequency = pulseIn(C_OUT, LOW);
  // Remaping the value of the BLUE (B) frequency from 0 to 255
  RGB[2] = map(redFrequency, B_LOW, B_HIGH, 255,0);

  //disable TSC output
  digitalWrite(OE, HIGH);
}

/* Returns a color depending on the received color values.
 * possible colours:
 * BLACK, RED, YELLOW, GREEN, CYAN, BLUE, MAGENTA
 */
TSCcolor_e TSC_Color(uint8_t RGB[])
{
  TSCcolor_e result;
  uint8_t Red = RGB[0];
  uint8_t Green = RGB[1];
  uint8_t Blue = RGB[2];
  
  if (Red < 10 && Green < 10 && Blue < 10){
    result = BLACK;
  }
  else if(Red > 200 && (Green > 10 && Green < 50) && Blue < 10){
    result = RED;
  }
  else if(Red > 200 && Green > 200 && Blue < 50){
    result = YELLOW;
  }
  else if(Red < 10 && Green > 200 && Blue < 10){
    result = GREEN;
  }
  else if(Red < 40 && (Green > 100 && Green < 200) && Blue > 200){
    result = CYAN;
  }
  else if(Red < 10 && (Green > 50 && Green < 150) && Blue > 200){
    result = BLUE;
  }
  else if(Red > 200 && Green < 10 && (Blue > 10 && Blue <100)){
    result = MAGENTA;
  }  
  else{
    result = 0; //Color not identified
  }
  return result;
}

//nRF24L01 interrupt call
void nRF_IRQ() {
  noInterrupts();
  radio.whatHappened(b_tx_ok, b_tx_fail, b_rx_ready);
  interrupts();
}
