/* About the software

   Start-up for the Arduino Sensor/Actuator side of the project
   What it should do:
   receive commando's adressed to it's address,
   depending on the commando one of the following actions:

   0) do nothing / stop the current action
   1) read sensor value and use it for own actuator
   2) read sensor value and send to received address
   3) read received data and apply to actuator
   4) reset the node / recalibrate sensor & actuator

   flow altering defined variables:
   DEBUG
        enables all the debugging functionality and sends informational data over Serial COM port to an connected PC (baud 115200)
   CONTINIOUS
        enables the node to to excecute a given command until an other command is received.
        if CONTINIOUS is not active a command is excecuted once after the command is received.
   INTERRUPT
        Enables the interrupt sequence so the nRF module alerts the controller if data is available.
        If disabled the controller wille poll the nRF module for data. (Irritating child...)
*/
#define DEBUG
#define CONTINIOUS
#define INTERRUPT

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RoboCore_MMA8452Q.h>
#include <FastLED.h>
#include <hsv2rgb.h>

#ifdef DEBUG
#include <printf.h>
#endif

/*Variables for the nRF module*/
RF24 radio(7, 6, 5000000); //CE, CSN
const byte localAddr = 1; //node x in systeem // node 0 is masternode
const uint32_t listeningPipes[5] = {0x3A3A3AA1UL, 0x3A3A3AB1UL, 0x3A3A3AC1UL, 0x3A3A3AD1UL, 0x3A3A3AE1UL};
bool b_tx_ok, b_tx_fail, b_rx_ready = 0;

/* Datapaket standaard.
   datapaketten verzonden binnen dit project zullen dit formaat hanteren om een uniform systeem te vormen
   destAddr     //adres (4x8bits) ontvangen met pakket, zal volgens commando een ontvangend adres worden of een adres waarnaar gezonden word
   intensity    //intensity of the LEDs
   hue          //color of the LEDs transcoded in a hue
   saturation   //saturation of the colors
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
  uint8_t intensity;
  uint8_t hue;
  uint8_t saturation;
  uint8_t command;
} dataIn, dataOut;

/*Variables for the MMA module*/
MMA8452Q accel;
uint8_t mappedReadings[2];
/*Variables for the LED*/
#define NUM_LEDS 10
#define DATA_PIN 1
#define numReadings 6

CRGB leds[NUM_LEDS];
CHSV hsv(0, 0, 0);

const int nRFint_pin = 2;
const int MMAint_pin = 3;

int16_t MMA_lowerLimit = -1200;
int16_t MMA_upperLimit = 1200;

//defines the axis used for interaction
#define AXIS mappedReadings[0] //[0] = X, [1] = Y, [2] = Z


void setup() {
  pinMode(nRFint_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(nRFint_pin), nRF_IRQ, LOW);

#ifdef DEBUG
  Serial.begin(115200);
  printf_begin();
#endif

  //Initailisation of the nRF24L01 chip
  if(radio.begin())
	Serial.println("radio initialised");
	
  radio.setAddressWidth(4);
  for (uint8_t i = 0; i < 4; i++)
    radio.openReadingPipe(i, listeningPipes[localAddr] + i);

  radio.setPALevel(RF24_PA_HIGH);

  if (accel.init(MMA8452Q_Scale::SCALE_2G, MMA8452Q_ODR::ODR_50))
    Serial.println("Accel Initialised");

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);

  //print all settings of nRF24L01
#ifdef DEBUG
  radio.printDetails();
#endif

  if (!accel.active())
	Serial.println("accel active");
	
  radio.startListening();
}

void loop() {
  uint8_t deskHue, deskSat, deskInt;
  uint8_t map_x, map_y, map_z;

#ifdef INTERRUPT  
/* Uitvoering op interrupt basis
 * commando wordt opgeslagen
 * en uitgevoerd tot een ander commando verzonden wordt
 * of uitvoering gebeurd enkel wanneer er een commando binnenkomt 
*/
#ifdef CONTINIOUS //Interrupt continious
	if(b_rx_ready){
		b_rx_ready = 0;
		radio.read(&dataIn, sizeof(dataIn));
#ifdef DEBUG
	  Serial.println("IRQ geweest");
	  printf("Current command: %d\n\r", dataIn.command);
#endif
  if(dataIn.hue != 0 || datain.hue != deskHue)
    deskHue = dataIn.hue;

  if(dataIn.saturation != 0 || dataIn.saturation != deskSat)
    deskSat = dataIn.daturation;

  if(dataIn.intensity != 0 || dataIn.intensity != deskInt)
    deskInt = dataIn.intensity;
    
	}//end fetch command

	switch (dataIn.command){
		case 0: 
			FastLED.clear();
			break;

		case 1: //read sensor and use for own actuator
			accelRead();
		
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
			//mapped raw value
			fill_solid(leds, NUM_LEDS, CHSV(dataIn.hue, AXIS, dataIn.intensity));
			break;

		case 2: // read sensor and send to other actuator
			dataOut.command = 3;
			accelRead();
			dataOut.saturation = AXIS; //sensor input
			dataOut.destAddr = listeningPipes[localAddr];

			radio.stopListening();
			radio.openWritingPipe(dataIn.destAddr);//set destination address
			radio.write(&dataOut, sizeof(dataOut));
#ifdef DEBUG
	printf("%ld", dataIn.destAddr);
	printf("\n\rdata send: %d\n\r", dataOut.saturation);
#endif
			radio.startListening();
			break;

		case 3: //receive sensor value and use for own actuator
#ifdef DEBUG
	printf("receiving address: %ld\n\r", dataIn.destAddr);
	printf("received data: %d %d %d\n\r", dataIn.hue, dataIn.saturation, dataIn.intensity);
#endif			
			fill_solid(leds, NUM_LEDS, CHSV(dataIn.hue, dataIn.saturation, dataIn.intensity));
			break;
		case 4:
#ifdef DEBUG
	Serial.print("reset node");
#endif
	  //reset node
	default:
	  //do nothing
		break;
#ifdef DEBUG
	printf("Display LED\n\r");
#endif
	
	FastLED.show();
	
/* delay om uitvoering te vertragen tot 40Hz
 * uitvoering wordt vertraagd met 25 ms 
 * We gaan er van uit dat al de rest wordt "instant" wordt uitgevoerd. 
 * Wanneer het programma werkt kan dit verfijnt worden
 */
	delay(24);
	}// end switch

#endif

/* Verloop uitvoering als er commando binnen komt.
 * Verloopt op interruptbasis om processing te verlagen
*/
#ifndef CONTINIOUS //Interrupt Single operation
	if(b_rx_ready){
		b_rx_ready = 0; 
		radio.read(&dataIn, sizeof(dataIn));
#ifdef DEBUG
	Serial.print("Incomming command: ");
	Serial.println(dataIn.command);
#endif

	switch (dataIn.command){
		case 0:
			FastLED.clear();
			break;

		case 1: //read sensor and use for own actuator
			accelRead();
			
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
			//mapped raw value
			fill_solid(leds, NUM_LEDS, CHSV(dataIn.hue, AXIS, dataIn.intensity)); //GRB	
			break;

		case 2: // read sensor and send to other actuator
			dataOut.command = 3;
			accelRead();
			dataOut.saturation = AXIS; //sensor input
			dataOut.destAddr = listeningPipes[localAddr];

			radio.stopListening();
			radio.openWritingPipe(dataIn.destAddr);//set destination address
			radio.write(&dataOut, sizeof(dataOut));
#ifdef DEBUG
	printf("%ld", dataIn.destAddr);
	printf("\n\rdata send: %d", dataOut.dataValue);
#endif
			radio.startListening();
			break;

		case 3: //receive sensor value and use for own actuator
#ifdef DEBUG
	printf("receiving address: %ld\n\r", dataIn.destAddr);
	printf("received data: %d\n\r", dataIn.dataValue);
#endif
      fill_solid(leds, NUM_LEDS, CHSV(dataIn.hue, dataIn.saturation, dataIn.intensity));
			break;
		case 4:
#ifdef DEBUG

#endif
			//reset node
		default:
			//do nothing
			break;
		}//end switch
		
#ifdef DEBUG
	printf("Display LED\r\n");
#endif
		FastLED.show();
	}//end fetch-command
	
#endif //endif CONTINIOUS

#endif //end INTERRUPT

/* Uitvoering op poll basis
 * commando wordt opgeslagen
 * en uitgevoerd tot een ander commando verzonden wordt 
*/
#ifndef INTERRUPT
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
			FastLED.clear();
			break;

		case 1: //read sensor and use for own actuator
			accelRead();
			
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
			//mapped raw value
			fill_solid(leds, NUM_LEDS, CHSV(dataIn.hue, AXIS, dataIn.intensity));
			break;

		case 2: // read sensor and send to other actuator
			dataOut.command = 3;
			accelRead();
			dataOut.saturation = AXIS; //sensor input
			dataOut.destAddr = listeningPipes[localAddr];

			radio.stopListening();
			radio.openWritingPipe(dataIn.destAddr);//set destination address
			radio.write(&dataOut, sizeof(dataOut));
#ifdef DEBUG
	printf("%ld", dataIn.destAddr);
	printf("\n\rdata send: %d", dataOut.dataValue);
#endif
			radio.startListening();
			break;

		case 3: //receive sensor value and use for own actuator
#ifdef DEBUG
	printf("receiving address: %ld\n\r", dataIn.destAddr);
	printf("received data: %d\n\r", dataIn.dataValue);
#endif
			fill_solid(leds, NUM_LEDS, CHSV(dataIn.hue, dataIn.saturation, dataIn.intensity));
			break;
		case 4:
#ifdef DEBUG
	Serial.print("reset node");
#endif
			//reset node
		default:
			//do nothing
			break;
			
		FastLED.show();
/* delay om uitvoering te vertragen tot 50Hz
 * uitvoering wordt vertraagd met 20 ms 
 * We gaan ervan uit dat al de rest wordt "instant" zordt uitgevoerd. 
 * Wanneer het programma werkt kan dit verfijnt worden
*/
		delay(20);
	}// end switch
#endif //end poll
} //end loop

//read values from accelerometer
void accelRead(void){
  	accel.read();
  	
/*	nauwkeurigheid lezing kan worden aangepast in deze functies.
	variatie in kleur staat hier meer in verband.
	grote nauwkeurigheid = subtiele variatie; lage nauwkeurigheid = grote variatie	
	
	map(var, fromLow, fromHigh, toLow, toHigh)
	door de fromLow en fromHigh aan te passen veranderd de nauwkeurigheid
	-2047 tot 2046 is de hoogste nauwkeurigheid en de kleinste verandering
*/
  	mappedReadings[0] = map(accel.raw_x, MMA_lowerLimit, MMA_upperLimit, 0, 255);
  	mappedReadings[1] = map(accel.raw_y, MMA_lowerLimit, MMA_upperLimit, 0, 255);
  	mappedReadings[2] = map(accel.raw_z, MMA_lowerLimit, MMA_upperLimit, 0, 255);
   }

void nRF_IRQ() {
  noInterrupts();
  radio.whatHappened(b_tx_ok, b_tx_fail, b_rx_ready);
  interrupts();
}
