#include "Arduino.h"
#include <SPI.h>
#include <csrd.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>
#include <Wire.h>
#include <EEPROM.h>

//#define DEBUG 1

#define apin A0

CSRD server;
RH_RF69 driver(10,2);
//RHReliableDatagram manager(driver, 1);

#define NUM_CARS         10
#define SPEED_UP_PIN     3
#define SPEED_DOWN_PIN   4
#define SELECT_PIN       5
#define RELEASE_PIN      6
#define LED_PIN          A1
#define RADIO_RST        A4
#define ANGLE_LIMIT      10

#define NUM_SERVERS 10
#define REG_TIMEOUT 2000 //2 secs
#define RC_TIMEOUT 5000
#define RC_KEEP_ALIVE 300 //ms
#define CAR_KEEP_ALIVE_TIMEOUT 5000 //ms
#define BLINK_RATE 1000
#define UNREG_CAR_TIMEOUT 4000 // if a car does not send any message in 5s then unreg.

byte servers[NUM_SERVERS];

//time to keep the release button pressed to free the car
#define RELEASE_PRESS_TIME 1000


typedef struct CARS{
  uint8_t carid;
  uint8_t senderid;
  bool registered;
  long lastmsg;    
};

CARS cars[NUM_CARS];

uint8_t recbuffer[MESSAGE_SIZE];
byte radioBuffer[RH_RF69_MAX_MESSAGE_LEN];
byte i;

uint8_t serverId=0;
bool newMessage=false;
uint8_t carsIdx=0;
long request_register_time = 7000;   // time to send a request for registration if no car is registered
int  request_register_time_step = 4;  // multiplier of request_register_time to request all cars to register.
long last_request_register_time = 0;
byte car;
byte idx;

long last_request_battery = 0;
int request_battery_time = 500;   
int request_battery_time_step = 20;

byte potpin = A7;  
int val=0;
int ang=0;/*angle mapped from the potentimeter*/
int lastang=1;/*last angle selected*/
byte direction=0;/*car direction*/
byte potspeed = A6;
int speed = 0;/*car speed*/
int printang = 0;
int printspeed = 0;
int lastspeed = 1;

//timers
// auto enunmeration 
long t1,t2;
// send RC registration
long t3,t4;
//keep alive timer
long tk = 0;
long tk_car = 0;
long act = 0;
int t;
long tblink = 0;

bool resolved = false;
bool sentreg = false;
bool select_pressed = false; /*button select was pressed*/
bool waiting_acquire = false;/*indicates we are acquiring a car*/
uint8_t car_index = 0;
uint8_t acquiring_car = 0; /*car index we are acquiring*/
bool car_acquired = false; /*indicates we have a car*/
long t_acquire = 0;
uint8_t select_index = 0;/*actual index of registered cars*/
bool release_pressed = false; /*button release was pressed*/
uint8_t midang = 90;/*middle servo angle*/
bool setparam = false; /*set parameter state set*/

int potmax;
int potmin;

uint8_t sp_counter = 100; //increase or decrease depending on the button pressed
bool speedpressed = false;
bool any_car_registered = false;
boolean r=false;//for the radio

void setup(){

  digitalWrite(LED_PIN, HIGH);
  delay(300);
  digitalWrite(LED_PIN, LOW);
  delay(300);
  digitalWrite(LED_PIN, HIGH);
  delay(300);
  digitalWrite(LED_PIN, LOW);
  
  #ifdef DEBUG
  Serial.begin(115200);
  Serial.setTimeout(500);
  delay(100);
  Serial.println("start");
  #endif
  
  i=0;
  pinMode(RADIO_RST,OUTPUT);
  digitalWrite(RADIO_RST,HIGH);
  delay(10);
  digitalWrite(RADIO_RST,LOW);
  //if (!server.init(&driver,&manager)){

/*
* Start the radio
* try 10 times if failed
*/
  
  for (byte a=0;a<10;a++){
    if (server.init(&driver,NULL)) {
        r=true;
        //driver.setTxPower(17);
        break;
    }
    delay(200);
  }

  if (!r){
    #ifdef DEBUG
    Serial.println("FAILED");
    #endif
    digitalWrite(LED_PIN, HIGH);
  }
  //if (!server.init(&driver,NULL)){
  //  Serial.println("FAILED");
  //}
  driver.setTxPower(13);
  driver.setModemConfig(RH_RF69::FSK_Rb250Fd250);

  carsIdx=0;
  car=0;
 
  last_request_register_time = millis(); 
  
  //server.sendBroadcastRequestRegister(serverId);  

  //setup push buttons
  pinMode(SPEED_UP_PIN, INPUT_PULLUP);
  pinMode(SPEED_DOWN_PIN, INPUT_PULLUP);
  pinMode(SPEED_UP_PIN, INPUT_PULLUP);
  pinMode(SELECT_PIN, INPUT_PULLUP);
  pinMode(RELEASE_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);  

  randomSeed(analogRead(apin));
  serverId = EEPROM.read(0);
  if (serverId != EEPROM.read(1)){
    //get a random value if the old one is not the same
    serverId = random(1 , 255);
  }

  //get potmax and min  
  potmin = word(EEPROM.read(3), EEPROM.read(4));
  potmax = word(EEPROM.read(5), EEPROM.read(6));
  
  if (potmin >= potmax){
    potmin = 40;
    potmax = 700; 
  }
  if (potmin < 0) potmin = 0;

  if (potmax > 1023) potmax = 1023;
  #ifdef DEBUG
  Serial.print("potmin: ");Serial.println(potmin);
  Serial.print("potmax: ");Serial.println(potmax);
  #endif
  
  t1 = millis();  
  t3 = t1 - RC_TIMEOUT;//just to guarantee we send it as fast as we can
  
  #ifdef DEBUG
  Serial.print("random id: ");Serial.println(serverId);
  #endif
  
  t = random(0, 1000);
  //retry timer
  #ifdef DEBUG
  Serial.print("random retry timer: ");Serial.println(t);
  #endif
  
  digitalWrite(LED_PIN, LOW);
}

void loop(){
  
  newMessage = server.readMessage();
  if (!resolved) resolveId();
  checkAcquire();
  releaseCar();
  
  act = millis();
  
  if (newMessage){
    checkServerEnum();    
    confirmAcquire();
    confirmRelease();    

    //if (server.isCarKeepAlive() && server.getServerId() == serverId && server.getId() == cars[car].carid){
    if (server.isCarKeepAlive()){
      tk_car = act;//renew last keep alive  
      #ifdef DEBUG
      Serial.print("keep alive ");Serial.print(cars[car].carid);Serial.print(" ");Serial.println(server.getServerId());      
      #endif    
    }    
  }

  if (resolved){
    sendRCRegistration();
  }
  if (car_acquired){

    doFineTunning();  
  	setSteering();
    if (!setparam){
      setSpeed1();
    }  	
    if (act - tk > RC_KEEP_ALIVE){
      server.sendRCKeepAlive(cars[car].carid,serverId);
      tk = millis();
      cars[car].lastmsg = tk;
    }
   
    /*
    if (act - tk_car > CAR_KEEP_ALIVE_TIMEOUT){
      
        server.sendAddressedActionMessage(cars[car].carid, serverId, MOTOR, AC_MOVE, 0, 0);    
        delay(20);
        server.sendCarRelease(cars[car].carid, serverId);
        car_acquired = false;
        waiting_acquire = false;
        digitalWrite(LED_PIN, LOW);
        
        Serial.print("Releasing car timeout ");Serial.println(cars[car].carid);
    }
    */
  }
  mainloop();  
  blinkLed();
  unregister();
}



void blinkLed(){
  
  if (!car_acquired){
    any_car_registered = false;
    for (i = 0; i < NUM_CARS; i++){
      if (cars[i].carid != 0 ){
        any_car_registered = true;
        break;
      }
    }
    if (any_car_registered) {
      if (act - tblink > BLINK_RATE){
        digitalWrite(LED_PIN,!digitalRead(LED_PIN));
        tblink = act;
      }
    }
    else if (!r){
      digitalWrite(LED_PIN,LOW);    
    }
  }
}

bool doFineTunning(){
  if (digitalRead(SELECT_PIN) == LOW){
      setparam = false;
      if (digitalRead(SPEED_UP_PIN) == LOW){
        midang++;
        if (midang > 100) midang = 100;
        setparam = true;
      }
      if (digitalRead(SPEED_DOWN_PIN) == LOW){
        midang--;
        if (midang < 80) midang = 80;
        setparam = true;
      }
      if (setparam){
        
        #ifdef DEBUG
        Serial.print("trimming: ");Serial.println(midang);
        #endif
        
        server.sendSaveParam(cars[car].carid, serverId, 1, midang);
        delay(100);
        return true;
      }
    }  
    return false;
}

bool confirmRelease(){
  if (server.isCarReleaseAck() && server.getServerId() == serverId && server.getId() == cars[car].carid){
       //car aquired
       car_acquired = false;
       waiting_acquire = false;
       digitalWrite(LED_PIN, LOW);
       return true;
    }
    return false;
}

bool confirmAcquire(){
  if (server.isAcquireAck() && server.getServerId() == serverId && server.getId() == cars[acquiring_car].carid){
       //car aquired
       car_acquired = true;
       car = acquiring_car;
       waiting_acquire = false;
       digitalWrite(LED_PIN, HIGH);
       tk = act;//keep alive
       tk_car = tk;
       sp_counter = 101;
       return true;
    }
    return false;
}

void checkAcquire(){
  /* acquire a car */

  if (car_acquired) return;
  
  if (digitalRead(SELECT_PIN) == LOW){//pressed
    select_pressed = true;
  }
  else {
        if (select_pressed) {//was pressed and now released
            select_pressed = false;
            
            #ifdef DEBUG
            Serial.print("Acquired pressed ");Serial.println(select_index);
            #endif
            
        	  if (select_index > NUM_CARS) select_index = 0;
            
            i = select_index;
            uint8_t counter = 0;
            
            //for (i = select_index; i < NUM_CARS; i++){
            while (counter <= NUM_CARS){                
                if (select_index > NUM_CARS) select_index = 0;
                
          	    if (cars[select_index].carid != 0){
                   #ifdef DEBUG
                      Serial.print("selected idx ");Serial.println(select_index);
                      Serial.print("car ");Serial.println(cars[select_index].carid);
                      #endif
          	       if (waiting_acquire && select_index != acquiring_car){
          	       	  //the user pressed the button again. release any pending car
                                            
                      #ifdef DEBUG
                      Serial.print("Releasing car ");Serial.println(cars[acquiring_car].carid);
                      #endif
                                                                  
          		        server.sendCarRelease(cars[acquiring_car].carid, serverId);
                      delay(20);
          	       }
                   
                   #ifdef DEBUG
                   Serial.print("Acquire car ");Serial.println(cars[select_index].carid);
                   #endif
                   
          	       server.sendAcquire(cars[select_index].carid,serverId);
          	       waiting_acquire = true;
          	       t_acquire = millis();
          	       acquiring_car = select_index;
                   select_index++;
                   counter++;
                   break;
                   //select_index = i + 1;                   
                } 
                counter++;
                select_index++;             
    	      }	            
      }
  }
}

void releaseCar(){
  /* release the acquired car*/
  if (digitalRead(RELEASE_PIN) == LOW){//pressed
    if (car_acquired){
      release_pressed = true;
      #ifdef DEBUG
      Serial.println("release pressed");
      #endif
      return;      
    }

    if (digitalRead(SPEED_DOWN_PIN) == LOW){
      #ifdef DEBUG
      Serial.println("fine tunning start");
      #endif
      int potread;
      //not acquired do the calibration
      potread = analogRead(potpin);
      if (potread > potmax){
        potmax = potread;
      }
      if (potread < potmin){
        potmin = potread;
      }
      //save to eprom
      EEPROM.write(3,highByte(potmin));
      EEPROM.write(4,lowByte(potmin));
      EEPROM.write(5,highByte(potmax));
      EEPROM.write(6,lowByte(potmax));
      #ifdef DEBUG
      Serial.println("fine tunning done");
      #endif
    }
  }
  else {
    if (release_pressed) {//was pressed and now released
        #ifdef DEBUG
        Serial.println("Release pressed");
        #endif
        
        release_pressed = false;
        if (car_acquired){
          server.sendAddressedActionMessage(cars[car].carid, serverId, MOTOR, AC_MOVE, 0, 0);    
      	  delay(20);
      	  server.sendCarRelease(cars[car].carid, serverId);
      	  car_acquired = false;
          digitalWrite(LED_PIN, LOW);
          
          #ifdef DEBUG
          Serial.print("Releasing car ");Serial.println(cars[car].carid);
          #endif
	      }
    }
  }
}

void setSteering(){
  val = analogRead(potpin);
  /*
   * the map values depends on the potenciometers position
   * 12 degrees is limit for the car
   */
   
  ang=map(val, potmin, potmax, -ANGLE_LIMIT, ANGLE_LIMIT);
  if (ang > ANGLE_LIMIT) ang = ANGLE_LIMIT;
  if (ang < -ANGLE_LIMIT) ang = -ANGLE_LIMIT;  
  
  if (ang != lastang){       
    direction = 1;
    printang = ang;
    lastang = ang;
    if (ang < 0){
      direction = 0;
      ang = ang * -1;    
    }
    server.sendAddressedActionMessage(serverId, cars[car].carid, BOARD, AC_TURN, ang, direction); 
    delay(5); 
    //Serial.print("steering ");Serial.print(val);Serial.print("\t");Serial.println(ang);  
  }
  if (direction == 0){      
      ang = ang * -1;    
  }
}

void setSpeed(){
  val = analogRead(potspeed);
  speed=-1*map(val,40,650,-100,100);
  if (speed > 100) speed = 100;
  if (speed < -100) speed = -100;
  
  //Serial.print(val);Serial.print("\t");Serial.println(ang);
  if (speed != lastspeed){   
    direction = 1;
    lastspeed = speed;
    printspeed = speed;
    if (speed < 0){
      direction = 0;
      speed = speed * -1;    
    }
   
    server.sendAddressedActionMessage(serverId, cars[car].carid, MOTOR, AC_MOVE, speed, direction);  
    delay(20);  
  }
  if (direction == 1){      
      speed = speed * -1;    
  }
}

void setSpeed1(){
  speedpressed = false;
  if (digitalRead(SPEED_UP_PIN)){    
    sp_counter++;
    if (sp_counter > 201) sp_counter = 201;
    speedpressed = true;
  }

  if (digitalRead(SPEED_DOWN_PIN)){    
    sp_counter--;
    if (sp_counter < 1) sp_counter = 1;
    speedpressed = true;
  }  
  
  speed = sp_counter - 101;
  if (speed > 100) speed = 100;
  if (speed < -100) speed = -100;
  
  //Serial.print(val);Serial.print("\t");Serial.println(ang);
  if (speed != lastspeed){   
    direction = 0;
    lastspeed = speed;
    printspeed = speed;
    if (speed < 0){
      direction = 1;
      speed = speed * -1;    
    }
   
    server.sendAddressedActionMessage(serverId, cars[car].carid, MOTOR, AC_MOVE, speed, direction);  
    delay(10);  
  }
  if (direction == 1){      
      speed = speed * -1;    
  }
}


void mainloop(){
  #ifdef DEBUG
  //getSerialCommand();
  #endif
  //newMessage = server.readMessage();

  if (newMessage){
    /*
    Serial.println("New message");
    dumpMessage();
    Serial.println();
    Serial.print ("Reg: ");
    Serial.println(carsIdx);
    */
    if (server.isStatus()){
      
        #ifdef DEBUG
        Serial.println("St msg");
        #endif
        
        if (server.getStatusType() == RP_INITIALREG){
          byte idx=insertNode(server.getNodeNumber(),server.getSender());
          cars[idx].lastmsg = act;
           #ifdef DEBUG
           Serial.print("reg for ");
           Serial.println(cars[idx].carid);
           #endif
                      
           server.sendInitialRegisterMessage(cars[idx].carid,serverId,ACTIVE,255,255,255);
        }
        if (server.getStatusType() == STT_ANSWER_VALUE){
             #ifdef DEBUG
             Serial.print("St node ");
             Serial.println(server.getNodeNumber());
             Serial.print("e: ");
             Serial.print(server.getElement());
             Serial.print("\t p0: ");
             Serial.print(server.getVal0());
             Serial.print("\t p1: ");
             Serial.print(server.getVal1());
             Serial.print("\t p2: ");
             Serial.println(server.getVal2());  
             #endif
             
             byte idx = getCarIdx(server.getNodeNumber()); 
             if (idx != 255){
                //cars[idx].requests--;
             }
        }
        if (server.getStatusType() == STT_QUERY_VALUE_FAIL){
             #ifdef DEBUG
             Serial.println("Qry failed");
             #endif
        }
    }
    else {
      
    }
        
  }
 
  //sendRequestRegister();
  //requestBatterySpeedLevel(); 
  //unregister();
}

void checkServerEnum(){
  if (server.isResolutionId() && !resolved){
      servers[i] = server.getId();
      
      #ifdef DEBUG
      Serial.print ("received id: ");Serial.println(servers[i]);
      #endif
      
      i++;
      if (i > NUM_SERVERS) i = NUM_SERVERS -1;
    }
    if (server.isServerAutoEnum()){      
      server.sendId(serverId);
      #ifdef DEBUG
      Serial.print ("sent my id: ");Serial.println(serverId);
      #endif
    }
}

void resolveId(){
  //send autoenum message if not registered
  
  if (millis() - t1 > t){
    t1 = millis();
    server.sendServerAutoEnum(serverId);
  }
  if (!sentreg){
    t2 = millis();
    sentreg = true;
  }  

  if (millis() - t2 > REG_TIMEOUT){
    //end timer. transverse the data until find a valid id
    boolean f = true;
    #ifdef DEBUG
    Serial.println ("resolving the id");
    #endif
    while (f){
      f = false;
      for (byte j = 0; j < i; j++){
        if (servers[j] == serverId ){
          f = true;
          serverId ++;
          if (serverId > 255){
            serverId = 1;
          }
          break;
        }
      }
    }
    #ifdef DEBUG
    Serial.print ("valid id: ");Serial.println(serverId);
    #endif
    resolved = true;
    //save to eprom
    //save both value in the 2 first positions, so we can assure we did it
    EEPROM.write(0,serverId);
    EEPROM.write(1,serverId);
  }
}

void sendRCRegistration(){
  if (millis() - t3 > RC_TIMEOUT){
    t3 = millis();
    server.sendRCId(serverId);
    #ifdef DEBUG
    Serial.println("send RC reg");
    #endif
  }
}


void requestBatterySpeedLevel(){

  if (carsIdx == 0) return;

  if (cars[car].carid == 0) {
    car++;
    if (car>=carsIdx){
      car=0;
    }
    return;
  }

  long t = millis();
  if ((t - last_request_battery) > (request_battery_time * request_battery_time_step)){    

    #ifdef DEBUG
    Serial.print ("query ");
    Serial.println (cars[car].carid);   
    #endif
    
    server.sendAddressedStatusMessage(STT_QUERY_VALUE, serverId, cars[car].carid, BOARD, 0, 1, 2);       

    car++;
    if (car>=carsIdx){
      car=0;
    }    
    last_request_battery = millis();
  }
}

void unregister(){
  for (i = 0; i< NUM_CARS ;i++){
    if (cars[i].carid != 0 ){    
      if (act - cars[i].lastmsg > UNREG_CAR_TIMEOUT){
         cars[i].registered = false;
         cars[i].carid = 0;
         cars[i].lastmsg = 0;
         carsIdx--;
      }
    }
  }
}

void sendRequestRegister(){
  long t = millis();
  if ((t - last_request_register_time) > (request_register_time * request_register_time_step)){
     #ifdef DEBUG
     Serial.println("S b reg");
     #endif
     
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
    #ifdef DEBUG
    Serial.print (recbuffer[i]);
    Serial.print ("   ");
    #endif
  }
  #ifdef DEBUG
  Serial.println();
  #endif

}

uint8_t insertNode(uint16_t nn,int sender){
  byte i;

  if (carsIdx >= NUM_CARS){
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

void printBuffer(){
  int a;
  for (a=0;a<MESSAGE_SIZE;a++){
    #ifdef DEBUG
    Serial.print(recbuffer[a],HEX);
    Serial.print(" ");
    #endif
  }
}

#ifdef DEBUG

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
          Serial.println("S br wr");

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
               Serial.println("S Re");
              return server.sendRestoreDefaultConfig(serverId,nn,s);
            }
           
           return true;
          //return sendAddressedReadMessage(0,serverId,charToInt(serbuf[2]),charToInt(serbuf[3]));
      }

      if (serbuf[1] == 'O' || serbuf[1] == 'o') {
          //example: BO60 -> B=broadcast O=operation 6=element_motor 0=ON
          //Serial.println("C BO");
           Serial.println("S br op");
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
          Serial.println("S Br ac");
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
           Serial.println("S add wr");
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
           Serial.println("S add op");
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
            Serial.println("S Add ac");
               //bool sendAddressedActionMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t action,uint8_t val0,uint8_t val1);
               //bool sendAddressedActionMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t action,uint8_t val0,uint8_t val1);
             return server.sendAddressedActionMessage(serverId, nn, charToInt(serbuf[6]),charToInt(serbuf[5]),serbuf[7],p0);
        }
   } 
}
#endif
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





