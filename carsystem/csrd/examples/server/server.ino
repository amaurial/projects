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
uint8_t senders[128];
long register_time_interval = 1000;
long last_register_sent;
uint8_t serverId=1;
bool newMessage=false;
uint8_t carsIdx=0;
uint8_t turnOn=0;
long turnonffTime;
long turnonffWait=10000;
long request_register_time = 5000;   // time to send a request for registration if no car is registered
int  request_register_time_step = 30;  // multiplier of request_register_time to request all cars to register.
long last_request_register_time = 0;

void setup(){
  Serial.begin(115200);
  Serial.setTimeout(500);
  i=0;
   
  //if (!server.init(&driver,&manager)){
  if (!server.init(&driver,NULL)){
    Serial.println("FAILED");
  }
  driver.setModemConfig(RH_RF69::FSK_Rb250Fd250);
  count=0;
  carsIdx=0;
  status=ACTIVE;
  last_request_register_time = millis();
  Serial.println("START SERVER");
  Serial.println("Send broadcast register");
  server.sendBroadcastRequestRegister(0,serverId);  
}

void loop(){
  
  getSerialCommand();

  newMessage = server.readMessage();

  if (newMessage && server.isStatus()){
    Serial.println("New message");
    dumpMessage();
    Serial.println();
    int nn=insertNode(server.getNodeNumber(),server.getSender());    
    Serial.print("Confirming registration for ");
    Serial.println(cars[nn]);
    server.sendInitialRegisterMessage(senders[nn],serverId,ACTIVE,255,255,255);
    
    Serial.println("Message to restore eprom values ");
    server.sendRestoreDefaultConfig(serverId,cars[nn],senders[nn]);     
  }

  if (carsIdx>0 && (( millis()-turnonffTime)>turnonffWait)) {
    int i=0;
    
    for (i=0;i<carsIdx;i++){
      switch (turnOn){
        case (0):
          //Serial.print("Changing status for  ");
          //Serial.print(cars[i]);
          //Serial.println(" ON");
          //server.sendAddressedOPMessage(
           // senders[i],cars[i],MOTOR,ON,0,0);//motor on
           //delay(10);
           //server.sendAddressedOPMessage(
           //senders[i],cars[i],SIRENE_LIGHT,BLINKING,0,0);//sirene light on
          turnOn=1;
        break;
      
        case (1):
          //Serial.print("Changing status for  ");
          //Serial.print(cars[i]);
          //Serial.println(" EMERGENCY");
          //server.sendEmergencyBroadcast(serverId,1);
          turnOn=2;
        break;
        case (2):
          //Serial.print("Changing status for  ");
          //Serial.print(cars[i]);
          //Serial.println(" NORMAL");
          //server.sendBackToNormalBroadcast(serverId,1);
          turnOn=3;
        break;
        case (3):
          //Serial.print("Changing status for  ");
          //Serial.print(cars[i]);
          //Serial.println(" OFF");
          //server.sendAddressedOPMessage(
            //senders[i],cars[i],MOTOR,OFF,0,0);//motor on
           //delay(10);
           //server.sendAddressedOPMessage(
            //senders[i],cars[i],SIRENE_LIGHT,OFF,0,0);//sirene light on
          turnOn=0;
        break;
      }
     
    }
    turnonffTime=millis();
  }
  sendRequestRegister();
  

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

void sendRequestRegister(){
  long t = millis();
  if ((t - last_request_register_time) > (request_register_time * request_register_time_step)){
     Serial.println("Send broadcast register");
     server.sendBroadcastRequestRegister(0,serverId);
     last_request_register_time = millis();
  }
  else if(carsIdx == 0 && (t - last_request_register_time > request_register_time)){
     server.sendBroadcastRequestRegister(0,serverId);
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

bool getSerialCommand(){
   uint8_t serbuf[12];   
   uint16_t id=0;
   uint16_t val=0;   
   byte snid[3];

   if (Serial.available() > 0) {
       Serial.readBytesUntil('#', serbuf, 12);
       if (serbuf[0] == 'B'){
          if (serbuf[1] == 'W') {
              //example: BW60100 -> B=broadcast W=write 6=element_motor 0=param_index 100=speed%
	            //bool sendBroadcastWriteMessage(uint8_t serverAddr,uint8_t group,uint8_t element,uint8_t param_idx,uint8_t val0,uint8_t val1,uint8_t val2);
              snid[0]=serbuf[4];
	            snid[1]=serbuf[5];	
              snid[2]=serbuf[6];
              //Serial.println("C BW");
              
              return server.sendBroadcastWriteMessage(0,serverId,charToInt(serbuf[2]),charToInt(serbuf[3]),getNN(snid),0,0);
          }
          
          if (serbuf[1] == 'R') {
               //example: BR60 -> B=broadcast R=read 6=element_motor 0=param0
               //bool sendAddressedReadMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t param_idx);
               //Serial.println("C BR");
               return true;
	            //return sendAddressedReadMessage(0,serverId,charToInt(serbuf[2]),charToInt(serbuf[3]));
	        } 
                   
          if (serbuf[1] == 'O') {
              //example: BO60 -> B=broadcast O=operation 6=element_motor 0=ON
	            //Serial.println("C BO");
              
              return server.sendBroadcastOPMessage(serverId, 0, charToInt(serbuf[2]),charToInt(serbuf[3]),0,0,0);
          }  
                  
          if (serbuf[1] == 'C') {
              //Serial.println("C BC");
              
               return true; //need to implement
          }                    
       } 

       if (serbuf[0] == 'A'){
              snid[0]=serbuf[2];
              snid[1]=serbuf[3];  
              snid[2]=serbuf[4];
              id=getNN(snid);
              //Serial.print("ID:");
              //Serial.println(id);
          if (serbuf[1] == 'W') {
              //example: AW33360100 -> A=addressed W=write 333=node 6=element_motor 0=param_index 100=speed%
	            //bool sendAddressedWriteMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t param_idx,uint8_t val0,uint8_t val1);
              
              snid[0]=serbuf[7];
	            snid[1]=serbuf[8];	
              snid[2]=serbuf[9];
              //Serial.println("C AW");
              //Serial.print("PARAM:");
              //Serial.println(getNN(snid));
               
              return server.sendAddressedWriteMessage(0, id, charToInt(serbuf[5]),charToInt(serbuf[6]),getNN(snid),0);
          }
          if (serbuf[1] == 'R') {
               //example: AR33360 -> A=addressed R=read 333=node 6=element_motor 0=param0
               //bool sendAddressedReadMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t param_idx);
              //Serial.println("C AR");
               
	            return server.sendAddressedReadMessage(serverId,id,charToInt(serbuf[5]),charToInt(serbuf[6]));
	        }          
          if (serbuf[1] == 'O') {
              //example: AO33360 -> A=addressed O=operation 333=node 6=element_motor 0=ON
	            //bool sendAddressedOPMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t state,uint8_t val0,uint8_t val1);
              //Serial.println("C AO");
               
              return server.sendAddressedOPMessage(serverId,id, charToInt(serbuf[5]),charToInt(serbuf[6]),0,0);
          }          
          if (serbuf[1] == 'C') {
            //Serial.println("C AC");
               return true; //need to implement
          }                    
       } 
  
   }
   return false;
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













