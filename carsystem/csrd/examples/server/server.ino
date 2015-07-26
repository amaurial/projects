#include "Arduino.h"
#include <SPI.h>
#include <csrd.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>

CSRD server;
RH_RF69 driver;
//RHReliableDatagram manager(driver, 1);

uint8_t sbuffer[MESSAGE_SIZE];
uint8_t recbuffer[MESSAGE_SIZE];
byte radioBuffer[RH_RF69_MAX_MESSAGE_LEN];
byte i;
long st;
long count;

void setup(){
  Serial.begin(115200);
  i=0;
  //server.init(&driver,&manager);    
  //if (!server.init(&driver,&manager)){
  if (!server.init(&driver,NULL)){
    Serial.println("FAILED");
  }
  driver.setModemConfig(RH_RF69::FSK_Rb250Fd250);
  count=0;
  st=millis();
}

void loop(){

  if (i>254){
    i=0;
  }

  if (i>254){
      i=0;
    }

  
  if ((millis()-st)>5000){
    Serial.print("msg/s:");
    Serial.println(count/5);
    count=0;
    st=millis();
  }
 
  
  if (server.isRadioOn()){
    if (server.getMessage(recbuffer)){
       // Serial.print("from client ");
       // Serial.print(server.getSender());
        //Serial.print(": ");
       // printBuffer();
        //Serial.println();       
                
        setBuffer();
        server.sendMessage(sbuffer,MESSAGE_SIZE,server.getSender());
        count++;
        i++;;
    }
   
  }
}

void setBuffer(){
  sbuffer[0]=i;
  sbuffer[1]=44;
  sbuffer[2]=44;
  sbuffer[3]=44;
  sbuffer[4]=44;
  sbuffer[5]=44;
  sbuffer[6]=44;
  sbuffer[7]=i;
}
void printBuffer(){
  int a;
  for (a=0;a<MESSAGE_SIZE;a++){
    Serial.print(recbuffer[a],HEX);
    Serial.print(" ");
  }
}

