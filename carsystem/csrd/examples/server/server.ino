#include "Arduino.h"
#include <SPI.h>
#include <csrd.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>

CSRD server;
RH_RF69 driver;
RHReliableDatagram manager(driver, 1);

char buffer[8];
byte radioBuffer[RH_RF69_MAX_MESSAGE_LEN];
byte i;

void setup(){
  i=0;
  server.init(&driver,&manager);  
  Serial.begin(115200);
}

void loop(){

  if (i>254){
    i=0;
  }
  if (server.getMessage(buffer)){
      Serial.print("from client ");
      Serial.print(server.getSender());
      Serial.print(": ");
      Serial.println(buffer);
              
      setBuffer();
      server.sendMessage(buffer,8);
  }
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

