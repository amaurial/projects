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
STATUS status;
uint16_t cars[128];
int senders[128];
long register_time_interval = 1000;
long last_register_sent;
uint16_t serverId=1;
bool newMessage=false;
uint8_t carsIdx=0;
int turnOn=0;
long turnonffTime;
long turnonffWait=10000;

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
  carsIdx=0;
  status=ACTIVE;
  Serial.println("START");
}

void loop(){
  

  newMessage = server.readMessage();

  if (newMessage && server.isStatus()){
    dumpMessage();
    int nn=insertNode(server.getNodeNumber(),server.getSender());    
    
    server.sendInitialRegisterMessage(senders[nn],serverId,ACTIVE,0,0,0);

    Serial.print("Confirming registration for ");
    Serial.println(cars[nn]);    
    
  }

  if (carsIdx>0 && (( millis()-turnonffTime)>turnonffWait)) {
    int i=0;
    
    for (i=0;i<carsIdx;i++){
      switch (turnOn){
        case (0):
          Serial.print("Changing status for  ");
          Serial.print(cars[i]);
          Serial.println(" ON");
          server.sendAddressedOPMessage(
            senders[i],cars[i],MOTOR,ON,0,0);//motor on
           delay(10);
           server.sendAddressedOPMessage(
            senders[i],cars[i],SIRENE_LIGHT,BLINKING,0,0);//sirene light on
          turnOn=1;
        break;
      
        case (1):
          Serial.print("Changing status for  ");
          Serial.print(cars[i]);
          Serial.println(" EMERGENCY");
          server.sendEmergencyBroadcast(serverId,1);
          turnOn=2;
        break;
        case (2):
          Serial.print("Changing status for  ");
          Serial.print(cars[i]);
          Serial.println(" NORMAL");
          server.sendBackToNormalBroadcast(serverId,1);
          turnOn=3;
        break;
        case (3):
          Serial.print("Changing status for  ");
          Serial.print(cars[i]);
          Serial.println(" OFF");
          server.sendAddressedOPMessage(
            senders[i],cars[i],MOTOR,OFF,0,0);//motor on
           delay(10);
           server.sendAddressedOPMessage(
            senders[i],cars[i],SIRENE_LIGHT,OFF,0,0);//sirene light on
          turnOn=0;
        break;
      }
     
    }
    turnonffTime=millis();
  }
  
  

/*
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
  */
}

void dumpMessage(){
  int i=0;
  server.getMessageBuffer(recbuffer);
  for (i=0;i<MESSAGE_SIZE;i++){
    Serial.print (recbuffer[i]);
    Serial.print ("   ");
  }
  Serial.println();
  
}

uint8_t insertNode(uint16_t nn,int sender){
  int i;

  if (carsIdx==0){
    cars[0]=nn;
    senders[0]=sender;
    carsIdx++;
    return 0;
  }

  for (i=0;i<carsIdx;i++){
    if (cars[i]==nn){
      senders[i]=sender;
      return i;
    }
  }

  carsIdx++;
  cars[carsIdx]=nn;
  senders[carsIdx]=sender;
  return carsIdx-1;
  
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

