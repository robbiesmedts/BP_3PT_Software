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

/*Command enumeration*/
typedef enum sensorCommand : uint8_t{
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
   srcNode    //Enumeration van de node waar de data van origineerd
   destNode   //Enumeration van de node waar deze data voor bestemd is.
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

volatile byte nRF_Status;

RF24 radio(9, 10); //CE, CSN
const byte localAddr = 0; //node 0 is masternode
const uint32_t listeningPipes[5] = {0x3A3A3AA1UL, 0x3A3A3AB1UL, 0x3A3A3AC1UL, 0x3A3A3AD1UL, 0x3A3A3AE1UL};

int interrupt_pin = 2;

void setup() {
  pinMode(interrupt_pin, INPUT_PULLUP);

#ifdef DEBUG
  Serial.begin(115200);
  printf_begin();
#endif
  /*
     Initailisation of the nRF24L01 chip
  */
  radio.begin();
  radio.setAddressWidth(4);
  radio.setPALevel(RF24_PA_LOW);
  radio.openReadingPipe(0,listeningPipes[localAddr]);
  radio.startListening();

#ifdef DEBUG
  //print all settings of nRF24L01
  radio.printDetails();
  printf("data size: %d bytes\n\r", sizeof(dataStruct));
#endif
}

void loop() {
   //resetting DataStructure
   dataOut.srcNode = 0;
   dataOut.destNode = 0;
   dataOut.senCommand = disabled;
   dataOut.intensity = 0;
   dataOut.hue = 0;
   dataOut.saturation = 0;
   dataOut.sensorVal = 0;
   
   radio.stopListening();
   
// Testing basic HSI capabilities node

  radio.openWritingPipe(listeningPipes[1]);
  dataOut.srcNode = 0;
  dataOut.destNode = 1;
  dataOut.senCommand = disabled;
  
#ifdef DEBUG
  Serial.print("Commando 0 send to address: ");
  printf("%ld\n\r", listeningPipes[1]);
  Serial.println("increasing Intensity");
#endif
  //increasing intensity
  for (int i = 0; i<256; i++){
    dataOut.intensity = i;
    if (!radio.write(&dataOut, sizeof(dataStruct)))
    {
      Serial.println("transmission failed");
    }
    delay(20);
  }

#ifdef DEBUG
  Serial.println("Changing saturation");
#endif
  //Changing saturation
  for (int i = 0; i<256; i++){
    dataOut.saturation = i;
    if (!radio.write(&dataOut, sizeof(dataStruct)))
    {
      Serial.println("transmission failed");
    }
    delay(20);
  }

#ifdef DEBUG
  Serial.println("Changing hue");
#endif
  //Changing Hue
  for (int i = 0; i<256; i++){
    dataOut.hue = i;
    if (!radio.write(&dataOut, sizeof(dataStruct)))
    {
      Serial.println("transmission failed");
    }
    delay(20);
  }

#ifdef DEBUG
  Serial.print("Commando 1 send to address: ");
  printf("%ld\n\r", listeningPipes[1]);
  Serial.println("Activate sensor -> Hue");
  Serial.println("Hue 50%, Saturation 75%, Intensity 100%");
  Serial.println("Active for 20s");
#endif
//activate sensor -> Hue
  radio.openWritingPipe(listeningPipes[1]);
  dataOut.srcNode = 0;
  dataOut.destNode = 1;
  dataOut.senCommand = active_hue;
  dataOut.hue = 128;
  dataOut.intensity = 255;
  dataOut.saturation = 192;
  
  for (int i = 0; i<1000; i++){
    if (!radio.write(&dataOut, sizeof(dataStruct)))
    {
      Serial.println("transmission failed");
    }
    delay(20);
  }

#ifdef DEBUG
  Serial.print("Commando 1 send to address: ");
  printf("%ld\n\r", listeningPipes[1]);
  Serial.println("Activate sensor -> Saturation");
  Serial.println("Hue 50%, Saturation 75%, Intensity 100%");
  Serial.println("Active for 20s");
#endif
//activate sensor -> Saturation
  radio.openWritingPipe(listeningPipes[1]);
  dataOut.srcNode = 0;
  dataOut.destNode = 1;
  dataOut.senCommand = active_sat;
  dataOut.hue = 128;
  dataOut.intensity = 255;
  dataOut.saturation = 192;
  
  for (int i = 0; i<1000; i++){
    if (!radio.write(&dataOut, sizeof(dataStruct)))
    {
      Serial.println("transmission failed");
    }
    delay(20);
  }

#ifdef DEBUG
  Serial.print("Commando 1 send to address: ");
  printf("%ld\n\r", listeningPipes[1]);
  Serial.println("Activate sensor -> Intensity");
  Serial.println("Hue 25%, Saturation 75%, Intensity = 50%");
  Serial.println("Active for 20s");
#endif
//activate sensor -> intensity
  radio.openWritingPipe(listeningPipes[1]);
  dataOut.srcNode = 0;
  dataOut.destNode = 1;
  dataOut.senCommand = active_int;
  dataOut.hue = 64;
  dataOut.saturation = 192;
  dataOut.intensity = 128;
  
  for (int i = 0; i<1000; i++){
    if (!radio.write(&dataOut, sizeof(dataStruct)))
    {
      Serial.println("transmission failed");
    }
    delay(20);
  }

}//End loop()
