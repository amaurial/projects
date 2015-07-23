
#include "Arduino.h"
#include <SPI.h>
#include <csrd.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>

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


CSRD car;
RH_RF69 driver;
RHReliableDatagram manager(driver, car.getNodeNumber(),radioBuffer,RH_RF69_MAX_MESSAGE_LEN);
byte i=0;
byte radioBuffer[RH_RF69_MAX_MESSAGE_LEN];

void setup(){
  car.init(&driver,&manager); 
  i=0; 
  Serial.begin(115200);
}

void loop(){
  
  if (i>254){
    i=0;
  }
  setBuffer();

  if (car.isRadioOn()){
    server.sendMessage(buffer,8);
  }
  if (car.readMessage()){    
    
  }
  sleep(200);
  i++;
}
void setBuffer(){
  buffer[0]=i;
  buffer[1]=0;
  buffer[2]=0;
  buffer[3]=0;
  buffer[4]=0;
  buffer[5]=0;
  buffer[6]=0;
  buffer[7]=i;
}


