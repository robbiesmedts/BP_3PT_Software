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
*/
#define DEBUG
//#define CONTINIOUS
#define INTERRUPT

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RoboCore_MMA8452Q.h>
#include <FastLED.h>

#ifdef DEBUG
#include <printf.h>
#endif

/*Variables for the nRF module*/
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

/*Variables for the MMA module*/
MMA8452Q accel;
uint8_t mappedReadings[2];

/*Variables for the LED*/
#define NUM_LEDS 9
#define DATA_PIN 5
#define BRIGHTNESS 128
#define numReadings 6

CRGB leds[NUM_LEDS];

const int nRFint_pin = 2;
const int MMAint_pin = 3;

void setup() {

  pinMode(nRFint_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(nRFint_pin), nRF_IRQ, LOW);
  //pinMode(MMAint_pin, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(MMAint_pin), MMA_IRQ, LOW);

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

  if (accel.init(MMA8452Q_Scale::SCALE_2G, MMA8452Q_ODR::ODR_50))
    Serial.println("Accel Initialised");

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(BRIGHTNESS);

  if (!accel.active())
    Serial.println("accel active");

#ifdef DEBUG
  //print all settings of nRF24L01
  radio.printDetails();
#endif
  radio.startListening();
}

void loop() {
  uint8_t currentCommand;
  uint16_t currentValue;
  uint32_t currentDestAddr;
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
			fill_solid(CRGB(0, mappedReadings[1], 255)); //GRB
			break;

		case 2: // read sensor and send to other actuator
			dataOut.command = 3;
			accelRead();
			dataOut.dataValue = mappedReadings[1]; //sensor input
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
Serial.print("received data: %d\r\n", dataIn.dataValue);
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
#ifdef DEBUG
	printf("Display LED")
#endif
	
	FastLED.show();
/* delay om uitvoering te vertragen tot 50Hz
 * uitvoering wordt vertraagd met 20 ms 
 * We gaan ervan uit dat al de rest wordt "instant" zordt uitgevoerd. 
 * Wanneer het programma werkt kan dit verfijnt worden
 */
	delay(20);
	}// end switch

/* Verloop uitvoering als er commando binnen komt.
 * Verloopt op interruptbasis om processing te verlagen
*/
#else //Interrupt Single operation
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
			break;

		case 1: //read sensor and use for own actuator

#ifdef DEBUG

#endif

#ifdef DEBUG

#endif	
			break;

		case 2: // read sensor and send to other actuator

#ifdef DEBUG

#endif

			break;

		case 3: //receive sensor value and use for own actuator
#ifdef DEBUG

#endif

			break;
		case 4:
#ifdef DEBUG

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
			break;

		case 1: //read sensor and use for own actuator

#ifdef DEBUG

#endif 

#ifdef DEBUG

#endif 
			break;

		case 2: // read sensor and send to other actuator

#ifdef DEBUG

#endif

			break;

		case 3: //receive sensor value and use for own actuator
#ifdef DEBUG

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
		delay(20);
	}// end switch
#endif //endif INTERRUPT
} //end loop

//read values from accelerometer
void accelRead(void){
  	accel.read();
  	
/*	nauwkeurigheid lezing kan worden aangepast in deze functies.
	variatie in kleur staat hier meer in verband.
	grote nauwkeurigheid = subtiele variatie; lage nauwkeurigheid = grote variatie	
	
	map(var, fromLow, fromHigh, toLow, toHigh)
	door de fromLow en fromHigh aan te passen veranderd de nauwkeurigheid
	-2047 tot 2046 is de hoogste nauwkeurigheid
*/
  	mappedReadings[0] = map(accel.raw_x, -2000, 2000, 0, 255);
  	mappedReadings[1] = map(accel.raw_y, -2000, 2000, 0, 255);
  	mappedReadings[2] = map(accel.raw_z, -2000, 2000, 0, 255);
   }

void nRF_IRQ() {
  noInterrupts();
  radio.whatHappened(b_tx_ok, b_tx_fail, b_rx_ready);
  interrupts();
}

void MMA_IRQ() {
  noInterrupts();
  accel.read();
  interrupts();
}
