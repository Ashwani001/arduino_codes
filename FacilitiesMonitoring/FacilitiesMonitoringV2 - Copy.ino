/*========Hardware Related Macros=====================*/
#define         MQ_PIN                       (1)     //define which analog input channel you are going to use
#define         RL_VALUE                     (20)    //define the load resistance on the board, in kilo ohms
#define         RO_CLEAN_AIR_FACTOR          (10)    //RO_CLEAR_AIR_FACTOR=(Sensor resistance in clean air)/RO,
                                                     //which is derived from the chart in datashee 
/*============Software Related Macros ===============*/
#define         CALIBARAION_SAMPLE_TIMES     (50)    //define how many samples you are going to take in the calibration phase
#define         CALIBRATION_SAMPLE_INTERVAL  (500)   //define the time interal(in milisecond) between each samples in the
                                                     //cablibration phase
#define         READ_SAMPLE_INTERVAL         (50)    //define how many samples you are going to take in normal operation
#define         READ_SAMPLE_TIMES            (5)     //define the time interal(in milisecond) between each samples in 
                                                     //normal operation
 
/*=============Application Related Macros============*/
#define         GAS_LPG                      (0)
#define         GAS_CH4                      (1)
 
/*===============Globals=============================*/
float           LPGCurve[3]  =  {3,   0,  -0.4};    //two points are taken from the curve. 
                                                    //with these two points, a line is formed which is "approximately equivalent"
                                                    //to the original curve. 
                                                    //data format:{ x, y, slope}; point1: (lg1000, lg1), point2: (lg10000, lg0.4) 
float           CH4Curve[3]  =  {3.3, 0,  -0.38};   //two points are taken from the curve. 
                                                    //with these two points, a line is formed which is "approximately equivalent" 
                                                    //to the original curve.
                                                    //data format:{ x, y, slope}; point1: (lg2000, lg1), point2: (lg5000,  lg0.7) 
float           Ro           =  10;                 //Ro is initialized to 10 kilo ohms


#include <Servo.h>
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
#include "Adafruit_FONA.h"

#define FONA_RX 2
#define FONA_TX 10
#define FONA_RST 4
Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST);
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
#define WHITE 0x7
Servo myservo;
int reset;
void setup() {
  reset = 1;
   lcd.begin(16, 2);
    lcd.setBacklight(WHITE);
   lcd.setCursor(0, 0);
  lcd.clear();
  lcd.print("Calibrating...");
   Ro = MQCalibration(MQ_PIN); 
  
  pinMode(8,OUTPUT);
  pinMode(7,OUTPUT);
  pinMode(11,OUTPUT);
   myservo.attach(9);
   Serial.begin(4800);
   myservo.write(0); 
   
  // put your setup code here, to run once:

}

void loop() 
{
  while(reset)
  {
  int gas_lvl = g_lvl();//Reads gas level
  delay(50);
  float Temp = Temperature(); //Reads Temperature
  delay(50);

  if(gas_lvl > 500 || Temp>30) 
  {
  dcmotorControl(1);
  delay(50);
  stepperControl(1); 
  delay(50);
  buzzerOnOff(1);
  delay(50);
/*  sendmessage(1);*/
  delay(50);
  reset = 0;
 }
 
 if(Temp > 30)
 {
  
 sprinklerOnOFF(1);
    delay(50); 
     /* sendmessage(1);*/
  delay(50); 
    buzzerOnOff(1);
  delay(50);
 }
   if( Temp < 30 &&gas_lvl <500)
  {
    //stepperControl(0); 
    dcmotorControl(0);
    //sprinklerOnOFF(0);
    delay(50);
  }
  
 if(gas_lvl < 500)
  {
  //stepperControl(0); 
     delay(50);
  }
  if(gas_lvl >500)
  {
     stepperControl(1); 
     delay(50);
       /*sendmessage(1);*/
  delay(50);
    buzzerOnOff(1);
  delay(50);
  }
  


   /*
   * If temperature greater than critical
   * 1. Turn on the buzzer 1 --> Buzzer on
   * 2. Turn ON the water sprinkler 1 --> Sprinkler ON
   * 3. Send message via GSM module
   * 
   */

  
  delay(50);
// Display the Temperature and Gas_Lvl data on LCD all the time
  lcdDisplay(Temp, gas_lvl);
  Serial.println(gas_lvl);
  delay(50);
  }

}

// 1. Gas level sensing

int  MQGetPercentage(float rs_ro_ratio, float *pcurve)
{
  return (pow(10, (((log(rs_ro_ratio)-pcurve[1])/pcurve[2]) + pcurve[0])));
}

int MQGetGasPercentage(float rs_ro_ratio, int gas_id)
{
  if ( gas_id == GAS_LPG ) {
     return MQGetPercentage(rs_ro_ratio,LPGCurve);
  } else if ( gas_id == GAS_CH4 ) {
      return MQGetPercentage(rs_ro_ratio,CH4Curve);
  }    
 
  return 0;
}

float MQRead(int mq_pin)
{
  int i;
  float rs=0;
 
  for (i=0;i<READ_SAMPLE_TIMES;i++) {
    rs += MQResistanceCalculation(analogRead(mq_pin));
    delay(READ_SAMPLE_INTERVAL);
  }
 
  rs = rs/READ_SAMPLE_TIMES;
 
  return rs;  
}
float MQCalibration(int mq_pin)
{
  int i;
  float val=0;
 
  for (i=0;i<CALIBARAION_SAMPLE_TIMES;i++) {            //take multiple samples
    val += MQResistanceCalculation(analogRead(mq_pin));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  val = val/CALIBARAION_SAMPLE_TIMES;                   //calculate the average value
 
  val = val/RO_CLEAN_AIR_FACTOR;                        //divided by RO_CLEAN_AIR_FACTOR yields the Ro 
                                                        //according to the chart in the datasheet 
 
  return val; 
}

float MQResistanceCalculation(int raw_adc)
{
  return ( ((float)RL_VALUE*(1023-raw_adc)/raw_adc));
}

 float g_lvl()
{
float glevel;

glevel = MQGetGasPercentage(MQRead(MQ_PIN)/Ro,GAS_LPG);
return glevel;  
}
// 2. Temperature sensing
 float Temperature()
{
float Temp;
int tempPin = 0;

Temp = (5.0 * analogRead(tempPin) * 100.0) / 1024;

return Temp;  
}



//3. Servo control
void stepperControl(int x)
{
 
int pos = 0;
  
if(x == 1) 
{
 
  //for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(180);              // tell servo to go to position in variable 'pos'
   // delay(15);                       // waits 15ms for the servo to reach the position
  
}

else if(x == 0)
{
  //for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(0);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  //}

  
}

  
}
//4. DC motor control
void dcmotorControl(int x)
{
if(x ==1)
{
digitalWrite(8,HIGH);
  
}
else if(x ==0)
{
digitalWrite(8,LOW);
  
}
  
}
//5. Buzzer Control
void buzzerOnOff(int x)
{
if(x ==1)
{
digitalWrite(11, HIGH);
  
}
else if(x ==0)
{
digitalWrite(11, LOW);
  
}
  
}
//6. LED Sprinkler control
void sprinklerOnOFF(int x)
{
  if(x ==1)
{
digitalWrite(7, HIGH);
  
}
else if(x ==0)
{

 digitalWrite(7, LOW);
   
}
}
//7. LCD display
void lcdDisplay(int temp, int g_lvl )
{
  lcd.clear();
  lcd.setCursor(0, 0);
   lcd.print("Temperature: ");
   lcd.print(temp);
   lcd.setCursor(0, 1);
   lcd.print("LPG: ");
   lcd.print(g_lvl);
 // delay(1000);
//Code to display temperature and gas level

  
}
//8. GSM Trigger

void sendmessage()
{

     
 
}

