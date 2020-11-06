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
 *
 * nRF24L01 ||    Arduino
 * CE       ||    D9
 * CSN      ||    D10
 * MOSI     ||    D11
 * MISO     ||    D12
 * SCK      ||    D13
 * IQR      ||    D2
 * GND      ||    GND
 * VCC      ||    3V3
 *
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
 *
 *
 * flow altering definitions:
 * DEBUG
 *      enables all the debugging functionality and sends informational data over Serial COM port to an connected PC (baud 115200)
 * CONTINIOUS
 *      enables the node to to execute a given command until an other command is received.
 *      if CONTINIOUS is not active a command is executed once after the command is received.
 * INTERRUPT
 *    Enables the node to receive the nRF data on interrupt basis.
 *    This is less demanding for the processor, and releases some stress form the nRF network
 * NO_CONTROL
 *    This disables the nRF compatibility and the node becomes stand alone and always active
*/
#define DEBUG
#define CONTINIOUS
#define INTERRUPT
//#define NO_CONTROL

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
  
#ifdef DEBUG
  #include <printf.h>
#endif //DEBUG

#include <FastLED.h>
#include <hsv2rgb.h>

#define interrupt_pin 2
#define LED_pin 3
#define TRIG 5
#define ECHO 6

#define NUM_LEDS 60
#define COLOR_ORDER GRB
#define BRIGHTNESS 128
CRGB leds[NUM_LEDS];
CHSV hsv(0, 255, BRIGHTNESS);


#define MIN_DIST 0
#define MIN_DIST_CM 5
#define MAX_DIST 3500
#define MAX_DIST_CM 60

RF24 radio(9, 10); //CE, CSN
const byte localAddr = 2; //node x in systeem // node 0 is masternode
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
  uint32_t destAddr = listeningPipes[localAddr];
  uint16_t dataValue = 0;
  uint8_t command = 0;
} dataIn, dataOut;


void setup() {

#ifdef DEBUG
  Serial.begin(115200);
  printf_begin();
#endif

#ifdef INTERRUPT  
  pinMode(interrupt_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interrupt_pin), nRF_IRQ, LOW);
#endif

//Pin assignments for the ultrasone sensor
  pinMode(ECHO, INPUT);
  pinMode(TRIG, OUTPUT);
  digitalWrite(TRIG, LOW);
  
//Initialisation of the WS2812B LED
  FastLED.addLeds<WS2812B, LED_pin, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
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
  long l_distance;

#ifdef INTERRUPT  
/* Uitvoering op interrupt basis
 * commando wordt opgeslagen
 * en uitgevoerd tot een ander commando verzonden wordt
 * of uitvoering gebeurd enkel wanneer er een commando binnenkomt 
*/
#ifdef CONTINIOUS
  if(b_rx_ready){
    b_rx_ready = 0;
    radio.read(&dataIn, sizeof(dataIn));
#ifdef DEBUG
    Serial.println("IRQ geweest");
    printf("Current command: %d\n\r", dataIn.command);
#endif
  }//end fetch command

  switch (dataIn.command){
    case 0: //stop command, hold last value
      hsv.v = (uint8_t)dataIn.dataValue;
      for (uint8_t i = 0; i < NUM_LEDS; i++)
      hsv2rgb_rainbow(hsv, leds[i]);
      FastLED.show();
      break;

    case 1: //read sensor and use for own actuator
      l_distance = ultrasone();
      
      //limit the dsitance between the min and max distances
      l_distance = constrain(l_distance, MIN_DIST_CM, MAX_DIST_CM);
      //remap the mean distance to 0-255 for use with the hue functions
      hsv.hue = map(l_distance, MIN_DIST_CM, MAX_DIST_CM, 0, 255);
      
      for (uint8_t i = 0; i < NUM_LEDS; i++)
      hsv2rgb_rainbow(hsv, leds[i]);
#ifdef DEBUG
  Serial.print("Color in hue: ");
  Serial.println(hsv.hue);
#endif
      FastLED.show(); //display the LED output
#ifdef DEBUG
  Serial.println("transform to RGB and display LED");
#endif 
      break;

    case 2: // read sensor and send to other actuator
      dataOut.command = 3;
      l_distance = ultrasone();
      dataOut.dataValue = (uint16_t) l_distance; //sensor input
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
/* delay om uitvoering te vertragen tot 50Hz
 * uitvoering wordt vertraagd met 20 ms 
 * We gaan ervan uit dat al de rest wordt "instant" zordt uitgevoerd. 
 * Wanneer het programma werkt kan dit verfijnt worden
 */
  }// end switch
  delay(5);
/* Verloop uitvoering als er commando binnen komt.
 * Verloopt op interruptbasis om processing te verlagen
*/
#else //Single operation
  if(b_rx_ready){
    b_rx_ready = 0; 
    radio.read(&dataIn, sizeof(dataIn));
#ifdef DEBUG
  Serial.print("Incomming command: ");
  Serial.println(dataIn.command);
#endif

  switch (dataIn.command){
    case 0:
      //to be written
      for (uint8_t i = 0; i < NUM_LEDS; i++){
        leds[i] = CRGB::Black;
      }
      FastLED.show();
      break;

    case 1: //read sensor and use for own actuator
      l_distance = ultrasone();
      
      //limit the dsitance between the min and max distances
      l_distance = constrain(l_distance, MIN_DIST_CM, MAX_DIST_CM);
      //remap the mean distance to 0-255 for use with the hue functions
      hsv.hue = map(l_distance, MIN_DIST_CM, MAX_DIST_CM, 0, 255);
      
      for (uint8_t i = 0; i < NUM_LEDS; i++)
      hsv2rgb_rainbow(hsv, leds[i]);
#ifdef DEBUG
  Serial.print("Color in hue: ");
  Serial.println(hsv.hue);
#endif
      FastLED.show(); //display the LED output
#ifdef DEBUG
  Serial.println("transform to RGB and display LED");
#endif  
      break;

    case 2: // read sensor and send to other actuator
      l_distance = ultrasone();
      //cast to uint16_t
      dataOut.command = 3;
      dataOut.dataValue = (uint16_t) l_distance;
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
      //cast to float
      hsv.hue = (uint8_t) dataIn.dataValue;
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
  }//end non-continious
#endif //endif CONTINIOUS

#else
/* Uitvoering op poll basis
 * commando wordt opgeslagen
 * en uitgevoerd tot een ander commando verzonden wordt 
*/
  radio.whatHappened(b_tx_ok, b_tx_fail, b_rx_ready);
  if (b_rx_ready){
    radio.read(&dataIn, sizeof(dataIn));
#ifdef DEBUG
  Serial.println("data ontvangen");
  printf("Command: %d\n\r", dataIn.command);
#endif
  }
  
  switch (dataIn.command){
    case 0:
      //stop command, hold last value
      for (uint8_t i = 0; i < NUM_LEDS; i++)
        leds[i] = CRGB::Black;
      FastLED.show();
      break;

    case 1: //read sensor and use for own actuator
      l_distance = ultrasone();
      hsv.hue = map(l_distance, MIN_DIST, MAX_DIST, 0, 255);
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
      l_distance = ultrasone();
      dataOut.dataValue = (uint16_t) l_distance; //sensor input
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
  }// end switch
/* delay om uitvoering te vertragen tot 50Hz
 * uitvoering wordt vertraagd met 20 ms 
 * We gaan ervan uit dat al de rest wordt "instant" zordt uitgevoerd. 
 * Wanneer het programma werkt kan dit verfijnt worden
*/
  delay(10);
#endif //endif INTERRUPT
} //end loop

long ultrasone(void)
{
  uint32_t t1, t2, pulse_width;
  long l_distance_cm = 0;
  uint8_t i_distance_cm = 0;
  
  //trigger pulse >10us
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  
  //wait for echo pulse
  pulse_width = pulseIn(ECHO, HIGH);

  if (pulse_width > MAX_DIST){
    if(hsv.v != 0){
      hsv.v--;
    }
    else
    {
      hsv.v = 0;
    }
  }
  else{
    hsv.v = BRIGHTNESS;
  }
    
  // Calculate distance in centimeters and inches. The constants
  // are found in the datasheet, and calculated from the assumed speed 
  // of sound in air at sea level (~340 m/s).
  l_distance_cm = pulse_width / 58;

#ifdef DEBUG
      Serial.print("measured distance: ");
      Serial.println(l_distance_cm);
#endif
  return l_distance_cm;
}

#ifdef INTERRUPT
void nRF_IRQ() {
  noInterrupts();
  radio.whatHappened(b_tx_ok, b_tx_fail, b_rx_ready);
  interrupts();
}
#endif
