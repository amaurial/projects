#include "Arduino.h"
#include <SPI.h>
#include <csrd.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>
#include <Wire.h>
#include <EEPROM.h>

#define apin A0

CSRD server;
RH_RF69 driver(10,2);
//RHReliableDatagram manager(driver, 1);

#define NUM_CARS 20
#define NUM_SERVERS 10
#define REG_TIMEOUT 2000 //2 secs
#define RC_TIMEOUT 5000

byte cars[NUM_CARS];
byte servers[NUM_SERVERS];

//timers
// auto enunmeration 
long t1,t2;
// send RC registration
long t3,t4;
int t;
byte id = 0;
bool resolved = false;
bool sentreg = false;
byte i = 0;

void setup(){
  Serial.begin(115200);
  Serial.setTimeout(500);
  
/*
* Start the radio
* try 10 times if failed
*/
  boolean r=false;
  for (byte a=0;a<10;a++){
    if (server.init(&driver,NULL)) {
        r=true;
        //driver.setTxPower(17);
        break;
    }
    delay(200);
  }

  if (!r){
    Serial.println("FAILED");
  }
  driver.setModemConfig(RH_RF69::FSK_Rb250Fd250);
  randomSeed(analogRead(apin));
  id = EEPROM.read(0);
  if (id != EEPROM.read(1)){
    //get a random value if the old one is not the same
    id = random(1 , 255);
  }
  
  t1 = millis();  
  t3 = t1 - RC_TIMEOUT;//just to guarantee we send it as fast as we can
  Serial.print("random id: ");Serial.println(id);
  t = random(0, 1000);
  //retry timer
  Serial.print("random retry timer: ");Serial.println(t);
}

void loop(){
  
  if (!resolved) resolveId();
  
  if (server.readMessage()){
    checkServerEnum();
  }

  if (resolved){
    sendRCRegistration();
  }  
}

void checkServerEnum(){
  if (server.isResolutionId() && !resolved){
      servers[i] = server.getId();
      Serial.print ("received id: ");Serial.println(servers[i]);
      i++;
      if (i > NUM_SERVERS) i = NUM_SERVERS -1;
    }
    if (server.isServerAutoEnum()){      
      server.sendId(id);
      Serial.print ("sent my id: ");Serial.println(id);
    }
}

void resolveId(){
  //send autoenum message if not registered
  
  if (millis() - t1 > t){
    t1 = millis();
    server.sendServerAutoEnum(id);
  }
  if (!sentreg){
    t2 = millis();
    sentreg = true;
  }  

  if (millis() - t2 > REG_TIMEOUT){
    //end timer. transverse the data until find a valid id
    boolean f = true;
    Serial.println ("resolving the id");
    while (f){
      f = false;
      for (byte j = 0; j < i; j++){
        if (servers[j] == id ){
          f = true;
          id ++;
          if (id > 255){
            id = 1;
          }
          break;
        }
      }
    }
    Serial.print ("valid id: ");Serial.println(id);
    resolved = true;
    //save to eprom
    //save both value in the 2 first positions, so we can assure we did it
    EEPROM.write(0,id);
    EEPROM.write(1,id);
  }
}

void sendRCRegistration(){
  if (millis() - t3 > RC_TIMEOUT){
    t3 = millis();
    server.sendRCId(id);
  }
}


