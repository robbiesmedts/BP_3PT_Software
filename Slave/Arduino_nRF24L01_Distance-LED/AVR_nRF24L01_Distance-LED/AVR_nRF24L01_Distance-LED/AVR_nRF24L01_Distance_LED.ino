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
 * When the sensor is used, the received value represents the disctance to the sensor
 * 
 **About the hardware*
 *
 * detection distance and resolution: 2 - 450cm in steps of 0.3cm
 *
 * Software to be build for the Arduino Nano
 * input and output modules are:
 * - Velleman VMA306 ultrasone distance sensor
 * - RGB LED
 * 
 **How to connect the prototype*
 * VMA306   ||    Arduino
 * ECHO     ||    A2
 * TRIG     ||    A1
 * GND      ||    GND
 * VCC      ||    5V
 * 
 * LED      ||    Arduino
 * R        ||    D3
 * G        ||    D5
 * B        ||    D6
 * 
 * WS2812B  ||    Arduino
 * VCC      ||    5V
 * GND      ||    GND
 * DIn      ||    D3
 * 
 * flow altering defined variables:
 * DEBUG
 *      enables all the debugging functionality and sends informational data over Serial COM port to an connected PC (baud 115200)
 * CONTINIOUS
 *      enables the node to to excecute a given command until an other command is received.
 *      if CONTINIOUS is not active a command is excecuted once after the command is received.
*/
#define DEBUG
#define CONTINIOUS


#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <FastLED.h>
#include <hsv2rgb.h>

#ifdef DEBUG
#include <printf.h>
#endif

#define interrupt_pin 2
#define LED_pin 3
#define TRIG A1
#define ECHO A2

#define NUM_LEDS 5
#define CHIPSET WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];
CHSV hsv(0, 255, 255);

#define BRIGHTNESS 128
#define MIN_DIST 0
#define MAX_DIST 450

RF24 radio(9, 10); //CE, CSN
const byte localAddr = 2; //node x in systeem // node 0 is masternode
const uint32_t listeningPipes[5] = {0x3A3A3AA1UL, 0x3A3A3AB1UL, 0x3A3A3AC1UL, 0x3A3A3AD1UL, 0x3A3A3AE1UL}; 
bool b_tx_ok, b_tx_fail, b_rx_ready = 0;
uint8_t currentCommand;
uint16_t currentValue;
uint32_t currentDestAddr;

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


void setup() {

#ifdef DEBUG
  Serial.begin(115200);
  printf_begin();
#endif
  
  pinMode(interrupt_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interrupt_pin), nRF_IRQ, LOW);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  digitalWrite(TRIG, LOW);

  FastLED.addLeds<CHIPSET, LED_pin, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(BRIGHTNESS);
  
  // Initailisation of the nRF24L01 chip
  radio.begin();
  radio.setAddressWidth(4);
  for (uint8_t i = 0; i < 3; i++)
  radio.openReadingPipe(i, listeningPipes[localAddr] + i);
  
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();

#ifdef DEBUG
  //print all settings of nRF24L01
  radio.printDetails();
#endif
}

void loop() {
  
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
      break;

    case 1: //read sensor and use fo own actuator
        hsv.hue = ultrasone();
        for (uint8_t i = 0; i < NUM_LEDS; i++)
        hsv2rgb_rainbow(hsv, leds[i]);
#ifdef DEBUG
      Serial.print("Color in hue: ");
      Serial.println(hsv.hue);
#endif 
        FastLED.show();
#ifdef DEBUG
      Serial.println("transform to RGB and display LED");
#endif 
    break;

    case 2: // read sensor and send to other actuator
      dataOut.command = 3;
      f_distance = ultrasone();
      dataOut.dataValue = (uint16_t) f_distance; //sensor input
      dataOut.destAddr = listeningPipes[localAddr];
        
      radio.stopListening();
      radio.openWritingPipe(dataIn.destAddr);//set destination address
      radio.write(&dataOut, sizeof(dataOut));
#ifdef DEBUG
      printf("%ld", dataIn.destAddr);
      Serial.print("\n\rdata send: ");
      Serial.println(dataOut.dataValue);
#endif
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
      //do something
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
    delay(20);
  }// end switch
#endif

/* Verloop uitvoering als er commando binnen komt.
 * Verloopt op interruptbasis om processing te verlagen
*/
#ifndef CONTINIOUS
/*  if(b_rx_ready){
    b_rx_ready = 0; 
    radio.read(&dataIn, sizeof(dataIn));
#ifdef DEBUG
    Serial.print("Incomming command: ");
    Serial.println(dataIn.command);
#endif

    switch (dataIn.command){
      case 0:
        analogWrite(LEDR_pin, 0);
        analogWrite(LEDG_pin, 0);
        analogWrite(LEDB_pin, 0);
        break;

      case 1: //read sensor and use fo own actuator
        i_distance = ultrasone();
        int distance = constrain((int)255*i_distance,0,255);
        LEDHue(i_distance);
        break;

      case 2: // read sensor and send to other actuator
        f_distance = ultrasone();
        //cast to uint16_t
        dataOut.command = 3;
        dataOut.dataValue = i_distance;
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
        //cast to float
        LEDHue(dataIn.dataValue);
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
  }//end non-interrupt*/
#endif  
} //end loop

uint8_t ultrasone(void)
{
  uint32_t t1, t2, pulse_width;
  long l_distance_cm;
  uint8_t i_distance_cm;
  
  //trigger pulse >10us
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  
  //wait for echo pulse
  while(digitalRead(ECHO) == 0);
  
  //measure echo pulse length
  t1 = micros();
  while(digitalRead(ECHO) == 1);
  t2 = micros();
  pulse_width = t2 - t1;
  
  //calculate distance in cm. constants are found in datasheet
  l_distance_cm = pulse_width / 58;

#ifdef DEBUG
      Serial.print("measured distance: ");
      Serial.println(l_distance_cm);
#endif

  i_distance_cm = map(l_distance_cm, MIN_DIST, MAX_DIST, 0, 255);
  return i_distance_cm;
}

void nRF_IRQ() {
  noInterrupts();
  radio.whatHappened(b_tx_ok, b_tx_fail, b_rx_ready);
  interrupts();
}