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
 **About the hardware*
 *
 * Software to be build for the Arduino Nano
 * input and output modules are:
 * - Velleman VMA325 color sensor
 * - SRD-05VDC relais module
 * 
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
#include <TimerOne.h>

#ifdef DEBUG
#include <printf.h>
#endif

RF24 radio(9, 10); //CE, CSN
const byte localAddr = 1; //node x in systeem // node 0 is masternode
const uint32_t listeningPipes[5] = {0x3A3A3AA1UL, 0x3A3A3AB1UL, 0x3A3A3AC1UL, 0x3A3A3AD1UL, 0x3A3A3AE1UL}; 
bool b_tx_ok, b_tx_fail, b_rx_ready = 0;
int g_count = 0;  //count the frequency
int g_array[3];   //store the RGB value
int g_flag = 0;   //filter of RGB queue
float g_SF[3];    //save the RGB scale factor

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

/* Pin definitions */
const int interrupt_pin = 2; //nRF24L01 interrupt

const int C_OUT = 3; //TCS230 color value interrupt
const int S0 = 4; //output scaling 1
const int S1 = 5; //output scaling 2
const int S2 = 6; //photodiode selection 1
const int S3 = 7; //photodiode selection 2
const int LED = A0;

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
  Timer1.initialize(); //default is 1s
  Timer1.attachInterrupt(TCS_Callback);
  attachInterrupt(digitalPinToInterrupt(C_OUT), TSC_Count, RISING); //INT1

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
    }//end switch
  }//end non-interrupt
#endif  
} //end loop

//init TSC230 and setting frequency.
void TSC_Init(){
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

  digitalWrite(S0, LOW);// OUTPUT FREQUENCY SCALING 2%
  digitalWrite(S1, HIGH);
  digitalWrite(LED, HIGH); // LOW = Switch ON the 4 LED's , HIGH = switch off the 4 LED's
}

// clear counter and increment flag
void TSC_WB(int Level0, int Level1){
  g_count = 0;
  g_flag ++;
  TSC_FilterColor(Level0, Level1);
  Timer1.setPeriod(1000000); //value in microseconds
}

/* Select the filter color
  LOW   LOW   Red
  LOW   HIGH  Blue
  HIGH  LOW   Clear (no filter)
  HIGH  HIGH  Green
*/
void TSC_FilterColor(int Level01, int Level02){
  if (Level01 != 0)
    Level01 = HIGH;
  if (Level02 != 0)
    Level02 = HIGH;
  digitalWrite(S2, Level01);
  digitalWrite(S3, Level02);
}

//nRF24L01 interrupt call
void nRF_IRQ() {
  noInterrupts();
  radio.whatHappened(b_tx_ok, b_tx_fail, b_rx_ready);
  interrupts();
}

//TSC230 interrupt call
void TSC_Count(){
  g_count++;
}

/* Timer1 callback interrupt
  select action based on g_flag
  saves the cycle counter
*/
void TSC_Callback(){
  switch (g_flag){
    case 0:
      Serial.println("->WB Start");
      TSC_WB(LOW, LOW);
      break;
    case 1:
      Serial.print("->Frequency R=");
      Serial.println(g_count);
      g_array[0] = g_count;
      TSC_WB(HIGH, HIGH);
      break;
    case 2:
      Serial.print("->Frequency G=");
      Serial.println(g_count);
      g_array[1] = g_count;
      TSC_WB(LOW, HIGH);
      break;
    case 3:
      Serial.print("->Frequency B=");
      Serial.println(g_count);
      Serial.println("->WB End");
      g_array[2] = g_count;
      TSC_WB(HIGH, LOW);
      break;
    default:
      g_count = 0;
      g_flag = 0;
      for (int i = 0; i < 3; i++)
        Serial.println(g_array[i]);

      g_SF[0] = 255.0 / g_array[0]; //R Scale factor
      g_SF[1] = 255.0 / g_array[1] ; //G Scale factor
      g_SF[2] = 255.0 / g_array[2] ; //B Scale factor

      Serial.print("R Scale factor: ");
      Serial.println(g_SF[0], 4);
      Serial.print("G Scale factor: ");
      Serial.println(g_SF[1], 4);
      Serial.print("B Scale factor: ");
      Serial.println(g_SF[2], 4);
      break;
  }
}
