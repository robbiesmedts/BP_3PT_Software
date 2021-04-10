/* About the software

  HARDWARE CONNECTIONS
    D1 -> WS2812B Din 
    D2 -> nRF24 IRQ
    D3 -> MMA INT1
    D4 -> MMA SDA
    D5 -> MMA SCL
    D6 -> nRF CS
    D7 -> nRF CE
    D8 -> nRF SCK
    D9 -> nRF24 MISO
    D10 -> nRF24 MOSI
    3V3 -> nRF VCC
    GND -> nRF GND, MMA GND, WS2812B GND
    VCC -> WS2812B 5V, 
  
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
//#define CONTINIOUS
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

/*Command enumeration*/
typedef enum sensorCommand{
  disabled = 0,
  active_hue,
  active_sat,
  active_int, 
  receive_hue,
  receive_sat,
  receive_int
}e_command;

/* Datapaket standaard.
   datapaketten verzonden binnen dit project zullen dit formaat hanteren om een uniform systeem te vormen
   srcNode		//Enumeration van de node waar de data van origineerd
   destNode		//Enumeration van de node waar deze data voor bestemd is.
   command      //commando (Enum) gestuctureerd volgens command table
   intensity    //intensity of the LEDs
   hue          //color of the LEDs transcoded in a hue
   saturation   //saturation of the colors
   sensorval    //sensor values to use in calculations sigend (8-bit, value tussen -128 tot 127)
*/
struct dataStruct {
  byte srcNode;
  byte destNode;
  e_command senCommand;
  uint8_t intensity;
  uint8_t hue;
  uint8_t saturation;
  int8_t sensorVal;
} dataIn, dataOut;

/*Variables for the nRF module*/
RF24 radio(7, 6, 5000000); //CE, CSN
const byte localAddr = 1;
const uint32_t listeningPipes[5] = {0x3A3A3AA1UL, 0x3A3A3AB1UL, 0x3A3A3AC1UL, 0x3A3A3AD1UL, 0x3A3A3AE1UL};
bool b_tx_ok, b_tx_fail, b_rx_ready = 0;

/*Variables for the MMA module*/
MMA8452Q accel;
int8_t mappedReadings[2];
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
uint8_t deskHue, deskSat, deskInt;

void setup() {
  pinMode(nRFint_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(nRFint_pin), nRF_IRQ, LOW);

#ifdef DEBUG
  SerialUSB.begin(115200);
  while(!Serial);
  SerialUSB.println("XIAO startup");
  printf_begin();
#endif

  //Initailisation of the nRF24L01 chip
  if(radio.begin()){SerialUSB.println("radio initialised");}
  
  radio.setAddressWidth(4);
  for (uint8_t i = 0; i < 4; i++){radio.openReadingPipe(i, listeningPipes[localAddr] + i);}

  radio.setPALevel(RF24_PA_HIGH);

  if (accel.init(MMA8452Q_Scale::SCALE_2G, MMA8452Q_ODR::ODR_50)){SerialUSB.println("Accel Initialised");}

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);

  //print all settings of nRF24L01
#ifdef DEBUG
  radio.printDetails();
  SerialUSB.printf("data size: %d bytes\n\r", sizeof(dataStruct));
#endif

  if (!accel.active()){SerialUSB.println("accel active");}
	
  radio.startListening();
}

void loop() {
	uint8_t calcSensorVal;
	float inBetween;
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
    SerialUSB.println("IRQ geweest");
    SerialUSB.printf("srcNode: %d\n\r", dataIn.srcNode);
    SerialUSB.printf("destNode: %d\n\r", dataIn.destNode);
    SerialUSB.printf("command: %d\n\r", dataIn.senCommand);
    SerialUSB.printf("HSI: %d, %d, %d\n\r", dataIn.hue, dataIn.saturation, dataIn.intensity);
#endif
		if (dataIn.srcNode == 0){
			deskHue = dataIn.hue;
			deskSat = dataIn.saturation;
			deskInt = dataIn.intensity;
		}
	}//end fetch command
	
	if(dataIn.destNode == localAddr){
		switch (dataIn.senCommand){
		case disabled: 
			fill_solid(leds, NUM_LEDS, CHSV(dataIn.hue, dataIn.saturation, dataIn.intensity));
			break;

		case active_hue: //read sensor and add it to the hue value of the lightingdesk
			accelRead();		
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
			//mapped raw value
			calcSensorVal = deskHue + AXIS;
			fill_solid(leds, NUM_LEDS, CHSV(calcSensorVal, dataIn.saturation, dataIn.intensity));
			break;

		case active_sat: // read sensor and add it to the saturation value of the desk
			accelRead();		
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
			//mapped raw value
			
			inBetween = deskSat + AXIS;
			if (inBetween > 0 && inBetween < 255)
				calcSensorVal = (uint8_t)inBetween;
			if (inBetween < 0)
				calcSensorVal = 0;
			if (inBetween > 255)
				calcSensorVal = 255;
			fill_solid(leds, NUM_LEDS, CHSV(dataIn.hue, calcSensorVal, dataIn.intensity));
			break;
			
		case active_int: // read sensor and add it to the saturation value of the desk
			accelRead();		
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
			//mapped raw value
			inBetween = deskSat + AXIS;
			if (inBetween > 0 && inBetween < 255)
			calcSensorVal = (uint8_t)inBetween;
			if (inBetween < 0)
			calcSensorVal = 0;
			if (inBetween > 255)
			calcSensorVal = 255;
			fill_solid(leds, NUM_LEDS, CHSV(dataIn.hue, dataIn.saturation, calcSensorVal));
			break;
		
		default:
#ifdef DEBUG
printf("case not implemented %d", dataIn.senCommand);
#endif
			break;			
		}// end switch
    
  FastLED.show();
/* delay om uitvoering te vertragen tot 40Hz
 * uitvoering wordt vertraagd met 25 ms 
 * We gaan er van uit dat al de rest "instant" wordt uitgevoerd. 
 * Wanneer het programma werkt kan dit verfijnt worden
 */
#ifdef DEBUG
  printf("Display LED\n\r");
#endif

  delay(24);
  
	}// end if
	if(dataIn.destnode != localAddr){ //data comes from or is destined to other node
		switch (dataIn.senCommand){
		case active_hue: //read sensor and add it to the hue value of the lightingdesk			
			
			dataOut.senCommand = receive_hue;
			accelRead();
			dataOut.sensorVal = AXIS; //sensor input
			dataOut.srcNode = localAddr;

			radio.stopListening();
			radio.openWritingPipe(listeningPipes[dataIn.destNode]);//set destination address
			radio.write(&dataOut, sizeof(dataOut));
#ifdef DEBUG
	printf("%ld", listeningPipes[dataIn.destNode]);
	printf("\n\rdata send: %d\n\r", dataOut.saturation);
#endif
			radio.startListening();

			fill_solid(leds, NUM_LEDS, CHSV(deskHue, deskSat, deskInt));
			
			break;

		case active_sat: // read sensor and add it to the saturation value of the desk
			dataOut.senCommand = receive_sat;
			accelRead();
			dataOut.sensorVal = AXIS; //sensor input
			dataOut.srcNode = localAddr;

			radio.stopListening();
			radio.openWritingPipe(listeningPipes[dataIn.destNode]);//set destination address
			radio.write(&dataOut, sizeof(dataOut));
#ifdef DEBUG
	printf("%ld", listeningPipes[dataIn.destNode]);
	printf("\n\rdata send: %d\n\r", dataOut.saturation);
#endif
			radio.startListening();

			fill_solid(leds, NUM_LEDS, CHSV(deskHue, deskSat, deskInt));
			break;
			
		case active_int: // read sensor and add it to the saturation value of the desk
			dataOut.senCommand = receive_int;
			accelRead();
			dataOut.sensorVal = AXIS; //sensor input
			dataOut.srcNode = localAddr;

			radio.stopListening();
			radio.openWritingPipe(listeningPipes[dataIn.destNode]);//set destination address
			radio.write(&dataOut, sizeof(dataOut));
#ifdef DEBUG
	printf("%ld", listeningPipes[dataIn.destNode]);
	printf("\n\rdata send: %d\n\r", dataOut.saturation);
#endif
			radio.startListening();

			fill_solid(leds, NUM_LEDS, CHSV(deskHue, deskSat, deskInt));
			break;
		
		case receive_hue:
			accelRead();		
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
			//mapped raw value
			calcSensorVal = deskHue + dataIn.sensorVal;
			fill_solid(leds, NUM_LEDS, CHSV(calcSensorVal, deskSat, deskInt));
			break;
			
		case receive_sat:
			accelRead();		
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
			//mapped raw value
			calcSensorVal = deskHue + dataIn.sensorVal;
			fill_solid(leds, NUM_LEDS, CHSV(deskHue, calcSensorVal, deskHue));
			break;
			
		case receive_int:
			accelRead();		
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
			//mapped raw value
			calcSensorVal = deskHue + dataIn.sensorVal;
			fill_solid(leds, NUM_LEDS, CHSV(deskHue, deskSat, calcSensorVal));
			break;
		}// end switch

#ifdef DEBUG
  printf("Display LED\n\r");
#endif

    FastLED.show();
/* delay om uitvoering te vertragen tot 40Hz
 * uitvoering wordt vertraagd met 25 ms 
 * We gaan er van uit dat al de rest "instant" wordt uitgevoerd. 
 * Wanneer het programma werkt kan dit verfijnt worden
 */
    delay(24);
	}//end else	

#endif

/* Verloop uitvoering als er commando binnen komt.
 * Verloopt op interruptbasis om processing te verlagen
*/
#ifndef CONTINIOUS //Interrupt Single operation
	if(b_rx_ready){
		b_rx_ready = 0;
		radio.read(&dataIn, sizeof(dataIn));
#ifdef DEBUG
    SerialUSB.println("IRQ geweest");
    SerialUSB.printf("srcNode: %d\n\r", dataIn.srcNode);
    SerialUSB.printf("destNode: %d\n\r", dataIn.destNode);
    SerialUSB.printf("command: %d\n\r", dataIn.senCommand);
    SerialUSB.printf("HSI: %d, %d, %d\n\r", dataIn.hue, dataIn.saturation, dataIn.intensity);
#endif
		if (dataIn.srcNode == 0){
			deskHue = dataIn.hue;
			deskSat = dataIn.saturation;
			deskInt = dataIn.intensity;
		}
	
		if(dataIn.destNode == localAddr){
			switch (dataIn.senCommand){
			case disabled: 
				fill_solid(leds, NUM_LEDS, CHSV(deskHue, deskSat, deskInt));
				break;

			case active_hue: //read sensor and add it to the hue value of the lightingdesk
				accelRead();		
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
				//mapped raw value
				calcSensorVal = deskHue + AXIS;
				fill_solid(leds, NUM_LEDS, CHSV(calcSensorVal, deskSat, deskInt));
				break;

			case active_sat: // read sensor and add it to the saturation value of the desk
				accelRead();		
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
				//mapped raw value
			
				inBetween = deskSat + AXIS;
				if (inBetween > 0 && inBetween < 255)
					calcSensorVal = (uint8_t)inBetween;
				if (inBetween < 0)
					calcSensorVal = 0;
				if (inBetween > 255)
					calcSensorVal = 255;
				fill_solid(leds, NUM_LEDS, CHSV(deskHue, calcSensorVal, deskInt));
				break;
			
			case active_int: // read sensor and add it to the saturation value of the desk
				accelRead();		
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
				//mapped raw value
				inBetween = deskSat + AXIS;
				if (inBetween > 0 && inBetween < 255)
				calcSensorVal = (uint8_t)inBetween;
				if (inBetween < 0)
				calcSensorVal = 0;
				if (inBetween > 255)
				calcSensorVal = 255;
				fill_solid(leds, NUM_LEDS, CHSV(dataIn.hue, dataIn.saturation, calcSensorVal));
				break;
		
			default:
#ifdef DEBUG
printf("case not implemented %d", dataIn.senCommand);
#endif
				break;			
			}// end switch
 
      FastLED.show();

#ifdef DEBUG
  Serial.println("Display LED");
#endif

		}// end if
		if(dataIn.destNode != localAddr){ //data comes from or is destined to other node
			switch (dataIn.senCommand){
			case active_hue: //read sensor and add it to the hue value of the lightingdesk			
			
				dataOut.senCommand = receive_hue;
				accelRead();
				dataOut.sensorVal = AXIS; //sensor input
				dataOut.srcNode = localAddr;

				radio.stopListening();
				radio.openWritingPipe(listeningPipes[dataIn.destNode]);//set destination address
				radio.write(&dataOut, sizeof(dataOut));
#ifdef DEBUG
	printf("%ld", listeningPipes[dataIn.destNode]);
	printf("\n\rdata send: %d\n\r", dataOut.saturation);
#endif
				radio.startListening();

				fill_solid(leds, NUM_LEDS, CHSV(deskHue, deskSat, deskInt));
			
				break;

			case active_sat: // read sensor and add it to the saturation value of the desk
				dataOut.senCommand = receive_sat;
				accelRead();
				dataOut.sensorVal = AXIS; //sensor input
				dataOut.srcNode = localAddr;

				radio.stopListening();
				radio.openWritingPipe(listeningPipes[dataIn.destNode]);//set destination address
				radio.write(&dataOut, sizeof(dataOut));
#ifdef DEBUG
	printf("%ld", listeningPipes[dataIn.destNode]);
	printf("\n\rdata send: %d\n\r", dataOut.saturation);
#endif
				radio.startListening();

				fill_solid(leds, NUM_LEDS, CHSV(deskHue, deskSat, deskInt));
				break;
			
			case active_int: // read sensor and add it to the saturation value of the desk
				dataOut.senCommand = receive_int;
				accelRead();
				dataOut.sensorVal = AXIS; //sensor input
				dataOut.srcNode = localAddr;

				radio.stopListening();
				radio.openWritingPipe(listeningPipes[dataIn.destNode]);//set destination address
				radio.write(&dataOut, sizeof(dataOut));
#ifdef DEBUG
	printf("%ld", listeningPipes[dataIn.destNode]);
	printf("\n\rdata send: %d\n\r", dataOut.saturation);
#endif
				radio.startListening();

				fill_solid(leds, NUM_LEDS, CHSV(deskHue, deskSat, deskInt));
				break;
		
			case receive_hue:
				accelRead();		
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
				//mapped raw value
				calcSensorVal = deskHue + dataIn.sensorVal;
				fill_solid(leds, NUM_LEDS, CHSV(calcSensorVal, deskSat, deskInt));
				break;
			
			case receive_sat:
				accelRead();		
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
				//mapped raw value
				calcSensorVal = deskHue + dataIn.sensorVal;
				fill_solid(leds, NUM_LEDS, CHSV(deskHue, calcSensorVal, deskHue));
				break;
			
			case receive_int:
				accelRead();		
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
				//mapped raw value
				calcSensorVal = deskHue + dataIn.sensorVal;
				fill_solid(leds, NUM_LEDS, CHSV(deskHue, deskSat, calcSensorVal));
				break;
			}// end switch
#ifdef DEBUG
  Serial.println("Display LED\n\r");
#endif

      FastLED.show();
		}//end else	
	}//end fetch command
	
#endif //end ifndef CONTINIOUS

#endif //end INTERRUPT

/* Uitvoering op poll basis
 * commando wordt opgeslagen
 * en uitgevoerd tot een ander commando verzonden wordt 
*/
#ifndef INTERRUPT
	radio.whatHappened(b_tx_ok, b_tx_fail, b_rx_ready);
	if(b_rx_ready){
		b_rx_ready = 0;
		radio.read(&dataIn, sizeof(dataIn));
#ifdef DEBUG
	  Serial.println("data available");
	  printf("Current command: %d\n\r", dataIn.senCommand);
#endif
		if (dataIn.srcNode == 0){
			deskHue = dataIn.hue;
			deskSat = dataIn.saturation;
			deskInt = dataIn.intensity;
		}
	}//end fetch command
	
	if(dataIn.destNode == localAddr){
		switch (dataIn.senCommand){
		case disabled: 
			fill_solid(leds, NUM_LEDS, CHSV(dataIn.hue, dataIn.saturation, dataIn.intensity));
			break;

		case active_hue: //read sensor and add it to the hue value of the lightingdesk
			accelRead();		
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
			//mapped raw value
			calcSensorVal = deskHue + AXIS;
			fill_solid(leds, NUM_LEDS, CHSV(calcSensorVal, dataIn.saturation, dataIn.intensity));
			break;

		case active_sat: // read sensor and add it to the saturation value of the desk
			accelRead();		
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
			//mapped raw value
			
			inBetween = deskSat + AXIS;
			if (inBetween > 0 && inBetween < 255)
				calcSensorVal = (uint8_t)inBetween;
			if (inBetween < 0)
				calcSensorVal = 0;
			if (inBetween > 255)
				calcSensorVal = 255;
			fill_solid(leds, NUM_LEDS, CHSV(dataIn.hue, calcSensorVal, dataIn.intensity));
			break;
			
		case active_int: // read sensor and add it to the saturation value of the desk
			accelRead();		
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
			//mapped raw value
			inBetween = deskSat + AXIS;
			if (inBetween > 0 && inBetween < 255)
			calcSensorVal = (uint8_t)inBetween;
			if (inBetween < 0)
			calcSensorVal = 0;
			if (inBetween > 255)
			calcSensorVal = 255;
			fill_solid(leds, NUM_LEDS, CHSV(dataIn.hue, dataIn.saturation, calcSensorVal));
			break;
		
		default:
#ifdef DEBUG
printf("case not implemented %d", dataIn.senCommand);
#endif
			break;			

			FastLED.show();
/* delay om uitvoering te vertragen tot 40Hz
 * uitvoering wordt vertraagd met 25 ms 
 * We gaan er van uit dat al de rest "instant" wordt uitgevoerd. 
 * Wanneer het programma werkt kan dit verfijnt worden
 */
#ifdef DEBUG
printf("Display LED\n\r");
#endif
			delay(24);
		}// end switch
	}// end if
	if(dataIn.destNode != localAddr){ //data comes from or is destined to other node
		switch (dataIn.senCommand){
		case active_hue: //read sensor and add it to the hue value of the lightingdesk			
			
			dataOut.senCommand = receive_hue;
			accelRead();
			dataOut.sensorVal = AXIS; //sensor input
			dataOut.srcNode = localAddr;

			radio.stopListening();
			radio.openWritingPipe(listeningPipes[dataIn.destNode]);//set destination address
			radio.write(&dataOut, sizeof(dataOut));
#ifdef DEBUG
	printf("%ld", listeningPipes[dataIn.destNode]);
	printf("\n\rdata send: %d\n\r", dataOut.saturation);
#endif
			radio.startListening();

			fill_solid(leds, NUM_LEDS, CHSV(deskHue, deskSat, deskInt));
			
			break;

		case active_sat: // read sensor and add it to the saturation value of the desk
			dataOut.senCommand = receive_sat;
			accelRead();
			dataOut.sensorVal = AXIS; //sensor input
			dataOut.srcNode = localAddr;

			radio.stopListening();
			radio.openWritingPipe(listeningPipes[dataIn.destNode]);//set destination address
			radio.write(&dataOut, sizeof(dataOut));
#ifdef DEBUG
	printf("%ld", listeningPipes[dataIn.destNode]);
	printf("\n\rdata send: %d\n\r", dataOut.saturation);
#endif
			radio.startListening();

			fill_solid(leds, NUM_LEDS, CHSV(deskHue, deskSat, deskInt));
			break;
			
		case active_int: // read sensor and add it to the saturation value of the desk
			dataOut.senCommand = receive_int;
			accelRead();
			dataOut.sensorVal = AXIS; //sensor input
			dataOut.srcNode = localAddr;

			radio.stopListening();
			radio.openWritingPipe(listeningPipes[dataIn.destNode]);//set destination address
			radio.write(&dataOut, sizeof(dataOut));
#ifdef DEBUG
	printf("%ld", listeningPipes[dataIn.destNode]);
	printf("\n\rdata send: %d\n\r", dataOut.saturation);
#endif
			radio.startListening();

			fill_solid(leds, NUM_LEDS, CHSV(deskHue, deskSat, deskInt));
			break;
		
		case receive_hue:
			accelRead();		
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
			//mapped raw value
			calcSensorVal = deskHue + dataIn.sensorVal;
			fill_solid(leds, NUM_LEDS, CHSV(calcSensorVal, deskSat, deskInt));
			break;
			
		case receive_sat:
			accelRead();		
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
			//mapped raw value
			calcSensorVal = deskHue + dataIn.sensorVal;
			fill_solid(leds, NUM_LEDS, CHSV(deskHue, calcSensorVal, deskHue));
			break;
			
		case receive_int:
			accelRead();		
#ifdef DEBUG
	printf("%d\t", map_x);
	printf("%d\t", map_y);
	printf("%d\n\r", map_z);
#endif
			//mapped raw value
			calcSensorVal = deskHue + dataIn.sensorVal;
			fill_solid(leds, NUM_LEDS, CHSV(deskHue, deskSat, calcSensorVal));
			break;
			
#ifdef DEBUG
	printf("Display LED\n\r");
#endif

		FastLED.show();
/* delay om uitvoering te vertragen tot 40Hz
 * uitvoering wordt vertraagd met 25 ms 
 * We gaan er van uit dat al de rest "instant" wordt uitgevoerd. 
 * Wanneer het programma werkt kan dit verfijnt worden
 */
		delay(24);
		}// end switch
	}//end else
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
  	mappedReadings[0] = map(accel.raw_x, MMA_lowerLimit, MMA_upperLimit, -128, 127);
  	mappedReadings[1] = map(accel.raw_y, MMA_lowerLimit, MMA_upperLimit, -128, 127);
  	mappedReadings[2] = map(accel.raw_z, MMA_lowerLimit, MMA_upperLimit, -128, 127);
}

void nRF_IRQ() {
  noInterrupts();
  radio.whatHappened(b_tx_ok, b_tx_fail, b_rx_ready);
  interrupts();
}
