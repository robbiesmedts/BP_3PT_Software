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

const int nRFint_pin = 2;
//const int MMAint_pin = 3;

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

  accel.init(MMA8452Q_Scale::SCALE_2G, MMA8452Q_ODR::ODR_50);
  accel.setFastRead(MMA8452Q_FastRead::FAST_READ_ON);
  //initMotion(64, 16); //threshold, counter
  
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

      case 1: //read sensor and use for own actuator
        accel.read();
		
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

//init Motion 
/*void initMotion(byte threshold, byte counter){
	if (threshold > 127) threshold = 127;
	
	//enable motion detection and axis interrupts
	byte cfg = accel.readRegister(MMA8452Q_Register::FF_MT_CFG);
	cfg |= (1 << 6); //OAE - Enable motion detection
	cfg |= (1 << 3); //XEFE - Enable interrupt flag on X-axis
	cfg &= (0 << 7); //ELE - Disable Flag latch
	accel.writeRegister(MMA8452Q_Register::FF_MT_CFG, cfg);
	
	//configure motion threshold
	byte ths = accel.readRegister(MMA8452Q_Register::FF_MT_THS);
	ths &= 0x80; //mask out THSn bits
	ths = threshold;
	ths |= (1<<7); // DBCNTM - 
	accel.writeRegister(MMA8452Q_Register::FF_MT_THS, ths);
	
	accel.writeRegister(MMA8452Q_Register::FF_MT_COUNT, counter);
	
	//configure Interrupt output
	byte cfg_int = accel.readRegister(MMA8452Q_Register::CTRL_REG5);
	cfg_int |= (1 << 2); //INT_CFG_FF_IMT - rout FF_MT interrupt to INT1
	accel.writeRegister(MMA8452Q_Register::CTRL_REG5, cfg_int);
	
	//enable Interrupt output
	byte en_int = accel.readRegister(MMA8452Q_Register::CTRL_REG4);
	en_int |= (1 << 2); //INT_EN_FF_MT - enable FF_MT interrupt
	accel.writeRegister(MMA8452Q_Register::CTRL_REG4, en_int);
}*/

void nRF_IRQ() {
  noInterrupts();
  radio.whatHappened(b_tx_ok, b_tx_fail, b_rx_ready);
  interrupts();
}

/*void MMA_IRQ() {
  noInterrupts();
  accel.read();
  interrupts();
}*/
