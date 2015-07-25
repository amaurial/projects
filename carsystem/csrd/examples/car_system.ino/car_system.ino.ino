
#include "Arduino.h"
#include <csrd.h>
#include <RHReliableDatagram.h>
#include <RH_RF69.h>
#include <SPI.h>

//PINS
#define FRONT_LIGHT               A1
#define LEFT_LIGHT_PIN            1
#define RIGHT_LIGHT_PIN           0
#define SIRENE_LIGHT_PIN          13 //AUX1
#define BREAK_LIGHT_PIN           2
#define BREAK_AUX_LIGHT_PIN       11
#define MOTOR_PIN                 A4
#define MOTOR_ROTATION_PIN        A3
#define BATTERY_PIN               A5
#define IR_REC_PIN                D5 //AUX2
#define IR_SENDER_PIN             D10 //AUX3


byte radioBuffer[RH_RF69_MAX_MESSAGE_LEN];
CSRD car;
RH_RF69 driver;
//RHReliableDatagram manager(driver, 33);

byte i=0;
byte j=0;
uint8_t sbuffer[MESSAGE_SIZE];
uint8_t recbuffer[MESSAGE_SIZE];
long st;
long count;

void setup(){
  Serial.begin(115200);
  //car.init(&driver,&manager); 
  //if (!car.init(&driver,&manager)){
  if (!car.init(&driver,NULL)){
    Serial.println("FAILED");
  }
  i=0; 
  count=0;
  st=millis();
}

void loop(){
  
  if (i>254){
    i=0;
  }

  
  if ((millis()-st)>5000){
    Serial.print("msg/s:");
    Serial.println(count/5);
    count=0;
    st=millis();
  }
  
    
   setBuffer();
   //Serial.println("Sending to the server");
   car.sendMessage(sbuffer,MESSAGE_SIZE,1); 

  for (j=0;j<10;j++){
    if (car.getMessage(recbuffer)>0){    
      //Serial.print("from server: ");
      //Serial.print(car.getSender());
      //Serial.print(" size: ");
      //Serial.println(car.getLength());
      //printBuffer();
      //Serial.println();
      count++;
      i++;
      break;
    }
    delay(1);
  }    
}
void setBuffer(){
  sbuffer[0]=i;
  sbuffer[1]=40;
  sbuffer[2]=40;
  sbuffer[3]=40;
  sbuffer[4]=40;
  sbuffer[5]=40;
  sbuffer[6]=40;
  sbuffer[7]=i;
}

void printBuffer(){
  int a;
  for (a=0;a<MESSAGE_SIZE;a++){
    Serial.print(recbuffer[a],HEX);
    Serial.print(" ");
  }
}


