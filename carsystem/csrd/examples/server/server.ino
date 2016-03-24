#include "Arduino.h"
#include <SPI.h>
#include <csrd.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>

CSRD server;
RH_RF69 driver;
//RHReliableDatagram manager(driver, 1);

#define NUM_CARS 10


typedef struct CARS{
  uint16_t carid;
  byte senderid;
  boolean registered;
  byte requests;
  byte idx;  
};

CARS cars[NUM_CARS];

uint8_t sbuffer[MESSAGE_SIZE];
uint8_t recbuffer[MESSAGE_SIZE];
byte radioBuffer[RH_RF69_MAX_MESSAGE_LEN];
byte i;
long st;

STATUS status;
long register_time_interval = 1000;
long last_register_sent;
uint8_t serverId=1;
bool newMessage=false;
uint8_t carsIdx=0;
uint8_t turnOn=0;
long turnonffTime;
long turnonffWait=10000;
long request_register_time = 5000;   // time to send a request for registration if no car is registered
int  request_register_time_step = 6;  // multiplier of request_register_time to request all cars to register.
long last_request_register_time = 0;
byte car;

long last_request_battery = 0;
int request_battery_time = 500;   
int request_battery_time_step = 20;


void setup(){
  Serial.begin(115200);
  Serial.setTimeout(500);
  i=0;

  //if (!server.init(&driver,&manager)){
  if (!server.init(&driver,NULL)){
    Serial.println("FAILED");
  }
  driver.setModemConfig(RH_RF69::FSK_Rb250Fd250);

  carsIdx=0;
  car=0;
  status=ACTIVE;
  last_request_register_time = millis();
  Serial.println("START SERVER");
  Serial.println("Send broadcast register");
  server.sendBroadcastRequestRegister(serverId);
}

void loop(){

  getSerialCommand();

  newMessage = server.readMessage();

  if (newMessage && server.isStatus()){
   // Serial.println("New message");
   // dumpMessage();
   // Serial.println();
    Serial.print ("Registered: ");
    Serial.println(carsIdx);
    
    if (server.getStatusType() == RP_INITIALREG){
      byte nn=insertNode(server.getNodeNumber(),server.getSender());
       Serial.print("registration for ");
       Serial.print(cars[nn].carid);
       Serial.print("\t");
       Serial.print(cars[nn].senderid);
       Serial.print("\t");
       Serial.println(server.getNodeNumber());
       
       server.sendInitialRegisterMessage(cars[nn].senderid,serverId,ACTIVE,255,255,255);
    }                

    
     if (server.getStatusType()==STT_ANSWER_VALUE){
         Serial.print("Status for node ");
         Serial.println(server.getNodeNumber());
         Serial.print("e: ");
         Serial.print(server.getElement());
         Serial.print("\t p0: ");
         Serial.print(server.getVal0());
         Serial.print("\t p1: ");
         Serial.print(server.getVal1());
         Serial.print("\t p2: ");
         Serial.println(server.getVal2());   
         byte idx = getCarIdx(server.getNodeNumber()); 
         if (idx != 255){
            cars[idx].requests--;
         }
     }
        
  }
 
  sendRequestRegister();
  requestBatterySpeedLevel(); 
  unregister();
}


void requestBatterySpeedLevel(){

  if (carsIdx == 0) return;

  if (cars[car].registered == false) {
    car++;
    if (car>=carsIdx){
      car=0;
    }
    return;
  }

  long t = millis();
  if ((t - last_request_battery) > (request_battery_time * request_battery_time_step)){    
     
    Serial.print ("query ");
    Serial.print (cars[car].carid);
    Serial.print (" ");
    Serial.println(cars[car].senderid);
    cars[car].requests++;
    server.sendAddressedStatusMessage(STT_QUERY_VALUE,cars[car].senderid,cars[car].carid,BOARD,0,1,2);       

    car++;
    if (car>=carsIdx){
      car=0;
    }
    
     last_request_battery = millis();
  }
}

void unregister(){
  for (byte i = 0; i< NUM_CARS ;i++){
    if (cars[i].requests > 6){
       cars[i].registered = false;
       cars[i].requests = 0;
       carsIdx--;
    }
  }
}

void sendRequestRegister(){
  long t = millis();
  if ((t - last_request_register_time) > (request_register_time * request_register_time_step)){
     Serial.println("Send broadcast register");
     server.sendBroadcastRequestRegister(serverId);
     last_request_register_time = millis();
  }
  else if(carsIdx == 0 && (t - last_request_register_time > request_register_time)){
     server.sendBroadcastRequestRegister(serverId);
     last_request_register_time = millis();
  }
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
  byte i;

  if (carsIdx > 9){
    return 255;
  }

  if (carsIdx == 0){
    cars[0].carid = nn;
    cars[0].senderid = sender;
    cars[0].registered = true;
    carsIdx++;
    return 0;
  }
  //check if exists and update the radio
  for (i=0;i<carsIdx;i++){
    if (cars[i].carid == nn){
      cars[i].senderid = sender;
      return i;
    }
  }
  
  cars[carsIdx].carid = nn;
  cars[carsIdx].senderid = sender;
  cars[carsIdx].registered = true;
  carsIdx++;
  return carsIdx-1;
}

uint8_t getSender(uint16_t nn){
   for (i=0;i<carsIdx;i++){
    if (cars[i].carid == nn){      
      return cars[i].senderid;
    }
  }
  return 255;
}

uint8_t getCarIdx(uint16_t nn){
   for (i=0;i<carsIdx;i++){
    if (cars[i].carid == nn){      
      return i;
    }
  }
  return 255;
}

void setBuffer(){
  for (i=0;i<8;i++){
    sbuffer[i]=0;
  }  
}
void printBuffer(){
  int a;
  for (a=0;a<MESSAGE_SIZE;a++){
    Serial.print(recbuffer[a],HEX);
    Serial.print(" ");
  }
}

bool getSerialCommand(){
   uint8_t serbuf[12];
   uint16_t id=0;
   uint16_t val=0;
   byte snid[3];

   if (Serial.available() == 0) return false;

   Serial.readBytesUntil('#', serbuf, 12);
   if (serbuf[0] == 'B' || serbuf[0] == 'b'){
      if (serbuf[1] == 'W' || serbuf[1] == 'w') {
          //example: BW60100 -> B=broadcast W=write 6=element_motor 0=param_index 100=speed%
          //bool sendBroadcastWriteMessage(uint8_t serverAddr,uint8_t group,uint8_t element,uint8_t param_idx,uint8_t val0,uint8_t val1,uint8_t val2);
          snid[0]=serbuf[4];
          snid[1]=serbuf[5];
          snid[2]=serbuf[6];
          Serial.println("Send broadcast write");

          return server.sendBroadcastWriteMessage(serverId,charToInt(serbuf[2]),charToInt(serbuf[3]),getNN(snid),0,0);
      }

      if (serbuf[1] == 'R' || serbuf[1] == 'r') {
           //example: BR60 -> B=broadcast R=read 6=element_motor 0=param0
           //bool sendAddressedReadMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t param_idx);
           //Serial.println("C BR");

            if (serbuf[2] == 'R' || serbuf[2] == 'r'){
              snid[0]=serbuf[3];
              snid[1]=serbuf[4];
              snid[2]=serbuf[5];
              int nn=getNN(snid);
              byte s=getSender(nn);
              if (s==255){
                s=0;
              }
               Serial.println("Send Restore");
              return server.sendRestoreDefaultConfig(serverId,nn,s);
            }
           
           return true;
          //return sendAddressedReadMessage(0,serverId,charToInt(serbuf[2]),charToInt(serbuf[3]));
      }

      if (serbuf[1] == 'O' || serbuf[1] == 'o') {
          //example: BO60 -> B=broadcast O=operation 6=element_motor 0=ON
          //Serial.println("C BO");
           Serial.println("Send broadcast operation");
          return server.sendBroadcastOPMessage(serverId, charToInt(serbuf[2]),charToInt(serbuf[3]),0,0,0);
      }

      if (serbuf[1] == 'C' || serbuf[1] == 'c') {
          //bc260100 = bc=broadcast action 2=set param 6=motor 0=speed 100=value
          snid[0]=serbuf[5];
          snid[1]=serbuf[6];
          snid[2]=serbuf[7];
          int v=getNN(snid);
          
          //Serial.println("C AW");
          //Serial.print("PARAM:");
          //Serial.println(getNN(snid));
          uint8_t p0,p1;
          
          if (v>255){
            p0=highByte(v);
            p1=lowByte(v);
          }
          else{
            p1=0;
            p0=v;
          }
          Serial.println("Send Broadcast action");
             //bool sendBroadcastActionMessage(uint8_t group,uint8_t element,uint8_t action,uint8_t val0,uint8_t val1,uint8_t val2);
           return server.sendBroadcastActionMessage(serverId, charToInt(serbuf[3]),charToInt(serbuf[2]),serbuf[4],p0,p1);
      }
   }

   if (serbuf[0] == 'A' || serbuf[0] == 'a'){
          snid[0]=serbuf[2];
          snid[1]=serbuf[3];
          snid[2]=serbuf[4];
          id=getNN(snid);
          //Serial.print("ID:");
          //Serial.println(id);
      if (serbuf[1] == 'W' || serbuf[1] == 'w') {
          //example: AW33360100 -> A=addressed W=write 333=node 6=element_motor 0=param_index 100=speed%
          //bool sendAddressedWriteMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t param_idx,uint8_t val0,uint8_t val1);

          snid[0]=serbuf[7];
          snid[1]=serbuf[8];
          snid[2]=serbuf[9];
          //Serial.println("C AW");
          //Serial.print("PARAM:");
          //Serial.println(getNN(snid));
          uint8_t p0,p1;
          int v=getNN(snid);
          if (v>255){
            p1=highByte(v);
            p0=lowByte(v);
          }
          else{
            p1=0;
            p0=v;
          }
           Serial.println("Send addressed write");
          return server.sendAddressedWriteMessage(0, id, charToInt(serbuf[5]),charToInt(serbuf[6]),p0,p1);
      }
      if (serbuf[1] == 'R' || serbuf[1] == 'r') {
           //example: AR33360 -> A=addressed R=read 333=node 6=element_motor 0=param0
           //bool sendAddressedReadMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t param_idx);
          //Serial.println("C AR");

          return server.sendAddressedReadMessage(serverId,id,charToInt(serbuf[5]),charToInt(serbuf[6]));
      }
      if (serbuf[1] == 'O' || serbuf[1] == 'o') {
          //example: AO33360 -> A=addressed O=operation 333=node 6=element_motor 0=ON
          //bool sendAddressedOPMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t state,uint8_t val0,uint8_t val1);
          //Serial.println("C AO");
           Serial.println("Send addressed operation");
          return server.sendAddressedOPMessage(serverId,id, charToInt(serbuf[5]),charToInt(serbuf[6]),0,0);
      }
        if (serbuf[1] == 'C' || serbuf[1] == 'c') {
            //Serial.println("C AC");
            //ac111260100 = ac=addressed action 111=node 2=AC_SET_PARAM 6=motor 0=speed 100=value

            snid[0]=serbuf[2];
            snid[1]=serbuf[3];
            snid[2]=serbuf[4];
            int nn=getNN(snid);
            
            snid[0]=serbuf[8];
            snid[1]=serbuf[9];
            snid[2]=serbuf[10];
            int v=getNN(snid);
            
            //Serial.println("C AW");
            //Serial.print("PARAM:");
            //Serial.println(getNN(snid));
            uint8_t p0,p1;
            
            if (v>255){
              p0=highByte(v);
              p1=lowByte(v);
            }
            else{
              p1=0;
              p0=v;
            }
            Serial.println("Send Addressed action");
               //bool sendAddressedActionMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t action,uint8_t val0,uint8_t val1);
               //bool sendAddressedActionMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t action,uint8_t val0,uint8_t val1);
             return server.sendAddressedActionMessage(serverId, nn, charToInt(serbuf[6]),charToInt(serbuf[5]),serbuf[7],p0);
        }
   } 
}

uint16_t getNN(byte *snn){
    String inString = "";
    for (byte a=0;a<3;a++){
	     inString += (char)snn[a];
    }
    return inString.toInt();
}

byte charToInt(byte v){
    return v - '0';
}













