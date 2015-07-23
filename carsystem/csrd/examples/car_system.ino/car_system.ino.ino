
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
RHReliableDatagram manager(driver, car.getNodeNumber());

void setup(){
  car.init(&driver,&manager);  
}

void loop(){
  if (car.readMessage()){
    
    
  }
}



