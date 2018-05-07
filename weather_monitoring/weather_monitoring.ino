/*
GSM -> Arduino
5V --> 5V
GND --> GND
RX --> TX1
TX --> RX1
GND --> GND
VMCU --> 5V
*/
#include "Adafruit_FONA.h"
#include <HardwareSerial.h>
#define fonaSS Serial1
HardwareSerial *fonaSerial = &fonaSS;

#define FONA_RX RX1
#define FONA_TX TX1
#define FONA_RST 4



/*------------------------------------------------------Including for sensors------------------------------------------------------*/
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <dht.h>
#include "DHT.h"
#define dht_apin A0

Adafruit_BMP085 bmp;
dht DHT;
const int sensorMin = 0;     // sensor minimum
const int sensorMax = 1024;  // sensor maximum
int LDR = 7;                 //analog pin to which LDR is connected, here we set it to 7 so it means A7
int LDRValue = 0;            //that’s a variable to store LDR values
int light_sensitivity = 460; //This is the approx value of light surrounding your LDR
/*------------------------------------------------------Including for sensors------------------------------------------------------*/

// this is a large buffer for replies
char replybuffer[255];

// Use this for FONA 800 and 808s
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
// Use this one for FONA 3G
//Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST);

int getNoOfSms();
void deleteMessage(uint8_t smsn);
void sendOutSMS(char *number, String msg, int8_t smsnum);
void handleAvailableMessages(int8_t smsnum);
String getMsg();
String getStatusMsg();
String print_status(String name, bool good);

uint8_t type;

void setup()
{

  while (!Serial);

  Serial.begin(4800);
  Serial.println(F("FONA basic test"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  fonaSerial->begin(4800);
  if (!fona.begin(*fonaSerial))
  {
    Serial.println(F("Couldn't find FONA"));
    while (1)
      ;
  }
  type = fona.type();
  Serial.println(F("FONA is OK"));
  Serial.print(F("Found "));
  switch (type)
  {
  case FONA800L:
    Serial.println(F("FONA 800L"));
    break;
  case FONA800H:
    Serial.println(F("FONA 800H"));
    break;
  case FONA808_V1:
    Serial.println(F("FONA 808 (v1)"));
    break;
  case FONA808_V2:
    Serial.println(F("FONA 808 (v2)"));
    break;
  case FONA3G_A:
    Serial.println(F("FONA 3G (American)"));
    break;
  case FONA3G_E:
    Serial.println(F("FONA 3G (European)"));
    break;
  default:
    Serial.println(F("???"));
    break;
  }

  // Print module IMEI number.
  char imei[16] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = fona.getIMEI(imei);
  if (imeiLen > 0)
  {
    Serial.print("Module IMEI: ");
    Serial.println(imei);
  }

  /*------------------------------------------------------Including for sensors------------------------------------------------------*/
  if (!bmp.begin())
  {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
  }
  
  delay(500); //Delay to let system boot
  Serial.println("Wired Communitcation\n\n");
  delay(1000);         //Wait before accessing Sensor
  //pinMode(13, OUTPUT); //we mostly use 13 because there is already a built in yellow LED in arduino which shows output when 13 pin is enabled
  /*------------------------------------------------------Including for sensors------------------------------------------------------*/
}


void loop()
{

  //Serial.print( getMsg() );
  //Serial.print( getStatusMsg() );

  int NoOfSms = getNoOfSms();
  if (NoOfSms > 0)
  {
    handleAvailableMessages(NoOfSms);
    Serial.println("[DEBUG]          Post handleAvaliableMsgds");
  }

  delay(5000);
}

String getMsg()
{
  String finalStr = "";
  //BMP180
  finalStr += "Temperature = ";
  finalStr += String(bmp.readTemperature());
  finalStr += " *C \n";

  finalStr +="Pressure = ";
  finalStr += String(bmp.readPressure());
  finalStr += (" Pa\n");

  // Serial.println();
  // delay(500);

  //DHT11 sensor
  DHT.read11(dht_apin);

  finalStr += "Current humidity = ";
  finalStr += String(DHT.humidity);
  finalStr += "%  ";
  finalStr +="temperature = ";
  finalStr += String(DHT.temperature);
  finalStr += "C  \n";
  // Serial.println();

  //RAIN SENSOR
  // read the sensor on analog A0:
  int sensorReading = analogRead(A1);
  // map the sensor range (four options):
  // ex: 'long int map(long int, long int, long int, long int, long int)'
  int range = map(sensorReading, sensorMin, sensorMax, 0, 2);

  // range value:
  switch (range)
  {
  case 0: // Sensor getting wet
    finalStr += "Rain\n";
    break;
  case 1: // Sensor getting wet
    finalStr += "Not Raining\n";
    break;
    // case 2:    // Sensor dry - To shut this up delete the " Serial.println("Not Raining"); " below.
    // Serial.println("Not Raining");
    //break;
  }
  // delay(500);  // delay between reads

  //LDR
  
  LDRValue = analogRead(LDR); //reads the ldr’s value through LDR

  //finalStr += String(LDRValue); //prints the LDR values to serial monitor
  //delay(50);        //This is the speed by which LDR sends value to arduino

  if (LDRValue < light_sensitivity)
  {
    finalStr += "Bright Day\n";
  }
  else
  {
    finalStr += "Cloudy Day\n";
  }

  return finalStr;
}


void deleteMessage(uint8_t smsn)
{
  Serial.print(F("\n\rDeleting SMS #"));
  Serial.println(smsn);
  if (fona.deleteSMS(smsn))
  {
    Serial.println(F("OK!"));
  }
  else
  {
    Serial.println(F("Couldn't delete"));
  }
}


int getNoOfSms()
{
  // read the number of SMS's!
  int8_t smsnum = fona.getNumSMS();
  if (smsnum < 0)
  {
    Serial.println(F("Could not read # SMS"));
  }
  else
  {
    Serial.print(smsnum);
    Serial.println(F(" SMS's on SIM card!"));
  }
  return smsnum;
}

void handleAvailableMessages(int8_t smsnum)
{
  uint16_t smslen;
  int8_t smsn;

  if ((type == FONA3G_A) || (type == FONA3G_E))
  {
    smsn = 0; // zero indexed
    smsnum--;
  }
  else
  {
    smsn = 1; // 1 indexed
  }

  for (; smsn <= smsnum; smsn++)
  {
    Serial.print(F("\n\rReading SMS #"));
    Serial.println(smsn);
    if (!fona.readSMS(smsn, replybuffer, 250, &smslen))
    {
      Serial.println(F("Failed!"));
    }
    if (smslen == 0)
    {
      Serial.println(F("[empty slot]"));
      smsnum++;
      continue;
    }

    if (strcmp(replybuffer, "/sendUpdate") == 0)
    {
      Serial.println("Time to send update!");
      String msgToSend = getMsg();
      char callerIDbuffer[32];
      if (fona.getSMSSender(smsn, callerIDbuffer, 31)) {
        fona.sendSMS(callerIDbuffer, msgToSend.c_str());
      }
      else{
        Serial.println("Could not get number to send sms!");
      }
    }

    else if (strcmp(replybuffer, "/checkStatus") == 0)
    {
      Serial.println("Checking status of sensors");
      String msgToSend = getStatusMsg();
      char callerIDbuffer[32];
      if (fona.getSMSSender(smsn, callerIDbuffer, 31)) {
        fona.sendSMS(callerIDbuffer, msgToSend.c_str());
      }
      else{
        Serial.println("Could not get number to send sms!");
      }
    }
      deleteMessage(smsn);
  }
}


String getStatusMsg()
{
  String StatusMsg = "";
  //BMP180 Temperature
  if( 18 < bmp.readTemperature() && bmp.readTemperature() < 30)
  {
    StatusMsg += print_status("bmp Temperature", true);
  }
  else{
    StatusMsg += print_status("bmp Temperature", false);
  }

  //BMP180 Pressure
  if( 90000 < bmp.readPressure() && bmp.readPressure() <  120000 )
  {
    StatusMsg += print_status("bmp Pressure", true);
  }
  else{
    StatusMsg += print_status("bmp Pressure", false);
  }

  //DHT11 sensor
  DHT.read11(dht_apin);
  //DHT11 sensor humidity
  if( 10.0 < DHT.humidity && DHT.humidity < 30.0 ) //TODO change value after getting new DHT
  {
    StatusMsg += print_status("DHT11 Humidity", true);
  }
  else{
    StatusMsg += print_status("DHT11 Humidity", false);
  }


  //RAIN SENSOR
  if( 0 <= analogRead(A1) && analogRead(A1) <= 1023 ) //TODO change value after getting new DHT
  {
    StatusMsg += print_status("Rain sensor", true);
  }
  else{
    StatusMsg += print_status("Rain sensor", false);
  }

  //LDR
  LDRValue = analogRead(LDR); 
  if( 0 <= analogRead(LDR) && analogRead(LDR) <= 1023 ) 
  {
    StatusMsg += print_status("Light sensor", true);
  }
  else{
    StatusMsg += print_status("Light sensor", false);
  }

  StatusMsg += "\n";

  return StatusMsg;
}


String print_status(String name, bool good){
  if(good){
    return name+": Good\n";
  }
  else{
    return name+": Bad\n";
  }
}
