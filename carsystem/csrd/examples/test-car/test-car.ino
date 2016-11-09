#include "Arduino.h"
#include <SPI.h>
#include <csrd.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>
#include <Wire.h>
#include <EEPROM.h>
#include <SoftPWM.h>
#include <SoftwareServo.h>

#define apin A0

//uncomment for debug message
//#define DEBUG_CAR 1;

CSRD car;
RH_RF69 driver(10,2);
//RHReliableDatagram manager(driver, 1);

#define NUM_CARS    10
#define NUM_SERVERS 5
#define REG_TIMEOUT 2000 //2 secs
#define RC_TIMEOUT  7000 //5 secs

uint8_t cars[NUM_CARS];
uint8_t servers[NUM_SERVERS];
uint8_t numrcs = 0;

/*
 * Io data
 */
#define BAT_FULL_LEVEL            610 //analog read equivalent to 4V
#define BAT_LEVEL_READ            680 //by tests 688 the motor stops
#define MOTOR_PIN                 A4
#define MOTOR_PIN1                A5
#define MOTOR_ROTATION_PIN        A6//A3
#define BATTERY_PIN               A7//A5
#define CHARGER_PIN               A1
#define STEERING_PIN              7
#define T_KEEP_ALIVE              1000 //ms
#define RC_TIMEOUT                2000//ms -receive from rc:w

uint8_t motorpin = MOTOR_PIN;
/*
* dynamic values
* [0] = batery
* [1] = motor
*/
#define D_VALUES 5
uint16_t dvalues[D_VALUES];
/*
 * Board params 5
 * 0 - adjust left
 * 1 - adjust right
 * 2 - 4 - spare
 */
 uint8_t boardParams[5] = {0, 0, 0, 0, 0};

//timers
// auto enunmeration 
unsigned long t1,t2;
// send RC registration
unsigned long t3;
// last rc message
unsigned long t4;
//send keep alive
unsigned long tk = 0;
//receive rc keep alive
unsigned long tk_rc = 0;
unsigned long act = 0;
unsigned long t;
unsigned long refresh_registration;
unsigned long last_registration;
uint8_t LAST_MESSAGE_TIMEOUT = 30; //30 seconds
unsigned long last_message = 0;
long counter = 0;
/*
 * battery vars
 */
long bat_send_timer = 0; //time between lowbat level

uint8_t id = 0;
bool resolved = false;
bool sentreg = false;
bool rc_connected = false;
uint8_t i = 0;


/*
 * steering variable
 * and call back function passed to softPWM
 * to refresh the soft servo lib
 */
SoftwareServo steering;
uint8_t midang = 90;
uint8_t lastAng = 0;
uint8_t lastspeed = 0;

volatile boolean acquired = false;
bool newMessage;
uint8_t rc;

//each 20ms
void refreshSoftServo(int a){  
  if (acquired){
    counter++;
    if (counter > 500){
      SoftwareServo::refresh();    
      counter = 0;
    }    
  }  
}

void setup(){
  #ifdef DEBUG_CAR
  Serial.begin(115200);
  Serial.setTimeout(500);
  #endif

  /*
   * Start steering
   */
  steering.attach(STEERING_PIN);
  steering.write(midang);  
  SoftwareServo::refresh();
  SoftPWMBegin(SOFTPWM_NORMAL,&refreshSoftServo);
  //SoftPWMBegin(SOFTPWM_NORMAL);
  for (byte i = 0; i < 10; i++){
    SoftwareServo::refresh();
    delay(20);
  }
  
  /*
  * Start the radio
  * try 10 times if failed
  */
  boolean r=false;
  for (uint8_t a=0;a<10;a++){
    if (car.init(&driver,NULL)) {
        r=true;
        //driver.setTxPower(17);
        break;
    }
    delay(200);
  }

  if (!r){
    #ifdef DEBUG_CAR
    Serial.println("FAILED");
    #endif
  }
  driver.setTxPower(13);
  driver.setModemConfig(RH_RF69::FSK_Rb250Fd250);
  randomSeed(analogRead(apin));
  id = EEPROM.read(0);
  if (id != EEPROM.read(1)){
    //get a random value if the old one is not the same
    id = random(1 , 255);
  }

  /*get the middle angle*/
  midang = EEPROM.read(2);
  if (midang < 80 || midang > 100){
    midang = 90; 
  }
            
  t1 = millis();  
  t3 = t1 - RC_TIMEOUT;//just to guarantee we send it as fast as we can

  #ifdef DEBUG_CAR
  Serial.print("random id: ");Serial.println(id);
  #endif
  t = random(0, 2000);
  //retry timer
  #ifdef DEBUG_CAR
  Serial.print("random retry timer: ");Serial.println(t);
  #endif

   /*
   * charger pin and reed pin 
   */
  pinMode(CHARGER_PIN, INPUT);  
  pinMode(BATTERY_PIN, INPUT);
  pinMode(MOTOR_ROTATION_PIN, INPUT);
  
  refresh_registration = random(200, 5000);
  
}

void loop(){  
  act = millis();
  SoftwareServo::refresh();
  if (!resolved) resolveId();

  if (resolved && !acquired){
    sendCarRegistration();
  }

  newMessage = car.readMessage();
  if (newMessage){        
    checkCarAutoEnum();
    #ifdef DEBUG_CAR
    Serial.print("pwrec ");Serial.println(driver.lastRssi());
    #endif
    if (car.isAcquire() && car.getId() == id && !acquired){
        //check if server is registered
        #ifdef DEBUG_CAR
        Serial.print("rec acquire numserv ");Serial.println(car.getServerId());
        #endif
        
        rc = car.getServerId();        
        for ( i = 0; i < NUM_SERVERS; i++){          
          if ( servers[i] == rc){            
            acquired = true;
            car.sendAcquireAck(id, rc);
            #ifdef DEBUG_CAR
            Serial.println("send acquire ack");
            #endif            
            steering.write(midang);
            steering.attach(STEERING_PIN);
            tk_rc = millis();            
            break;
          }
        }
    }
    if (acquired) {
      SoftwareServo::refresh();
      if (car.isRCKeepAlive() && car.getId() == id && rc == car.getServerId()){
         tk_rc = act; //renew the last keep alive
         #ifdef DEBUG_CAR
        //Serial.println("receive keep alive");
        #endif
      }
      
      if ((car.isCarRelease() && car.getId() == id) || (millis() - tk_rc > RC_TIMEOUT)){
        #ifdef DEBUG_CAR
        Serial.println("rec release");
        #endif
        SoftPWMSetPercent(MOTOR_PIN, 0);
        SoftPWMSetPercent(MOTOR_PIN1, 0);
        car.sendCarReleaseAck(id, rc);
        #ifdef DEBUG_CAR
        Serial.println("send release ack");
        #endif
        acquired = false;
      }
      
      if (checkAction()) tk_rc = act; 

      if (car.isSaveParam() && car.getId() == id && rc == car.getServerId()){
        #ifdef DEBUG_CAR
        Serial.print("trimming ");
        Serial.println(car.getParamIdx());
        #endif
        if (car.getParamIdx() == 1){
          midang = car.getVal0();
          #ifdef DEBUG_CAR
          Serial.print("trimming val ");Serial.println(midang);
          #endif
          if (midang >= 80 && midang <=100){
            steering.write(midang);  
          }
          else midang = 90;
          EEPROM.write(2,midang);          
        }
      }
      SoftwareServo::refresh();
      
      //checkQuery();      
    }    
  }

  if (acquired){
    SoftwareServo::refresh();
    /*
    if (millis() - tk > T_KEEP_ALIVE){
        tk = millis();        
        #ifdef DEBUG_CAR
        Serial.println("send keep alive");
        #endif
        car.sendCarKeepAlive(id, rc);    
        delay(5);    
      }   
      */   
  }
  else{
    steering.detach();
  }
   //checkBattery();
  //dvalues[1] = analogRead(MOTOR_ROTATION_PIN);    
   
}

void resolveId(){
  /*send autoenum message if not registered*/
  
  if (millis() - t1 > t){  
    t1 = millis();
    if (!sentreg){
      car.sendCarAutoEnum(id);
      sentreg = true;
      t2 = millis();
      #ifdef DEBUG_CAR
      Serial.println("send auto enum");
      #endif
    }
  }    

  if (millis() - t2 > REG_TIMEOUT){
    //end timer. transverse the data until find a valid id
    boolean f = true;
    
    #ifdef DEBUG_CAR
    Serial.println ("resolving the id");
    #endif
    
    while (f){
      f = false;
      for (uint8_t j = 0; j < i; j++){
        if (cars[j] == id ){
          f = true;
          id ++;
          if (id > 255){
            id = 1;
          }
          break;
        }
      }
    }

    #ifdef DEBUG_CAR
    Serial.print ("valid id: ");Serial.println(id);
    #endif
    
    resolved = true;
    //save to eprom
    //save both value in the 2 first positions, so we can assure we did it
    EEPROM.write(0,id);
    EEPROM.write(1,id);
  }
}
void checkCarAutoEnum(){
    
    if (car.isCarId() && !resolved){
      cars[i] = car.getId();

      #ifdef DEBUG_CAR
      Serial.print ("received id: ");Serial.println(servers[i]);
      #endif
      
      i++;
      if (i > NUM_CARS) i = NUM_CARS -1;
    }
    
    if (car.isCarAutoEnum()){      
      car.sendCarId(id);
      #ifdef DEBUG_CAR
      Serial.println("send id for autoenum");
      #endif
      
      #ifdef DEBUG_CAR
      Serial.print ("sent my id: ");Serial.println(id);
      #endif
      
    }

    if (car.isRCId()){
      bool rc_exist = false;
      uint8_t rc = car.getId();

      #ifdef DEBUG_CAR
      Serial.print ("rc rec ");Serial.println(rc);
      #endif
      
      for (i=0; i < NUM_SERVERS; i++){
        if (servers[i] == rc){
          rc_exist = true;
          #ifdef DEBUG_CAR
          Serial.println ("rc exist");
          #endif
          break;
        }
      }
      if (!rc_exist){
        servers[numrcs] = rc;
        #ifdef DEBUG_CAR
          Serial.print ("rc registered ");Serial.println(numrcs);
        #endif
        numrcs ++;
        if (numrcs >= NUM_SERVERS) {
          numrcs = NUM_SERVERS - 1;
        }
      }
    }
}

void sendCarRegistration(){
  if (millis() - t3 > RC_TIMEOUT){
    t3 = millis();
    if (!acquired){
      for (uint8_t j=0;j < numrcs; j++){
        car.sendInitialRegisterMessage(servers[j], id, ACTIVE, 0, 0, 0);
        #ifdef DEBUG_CAR
        Serial.println("send initial reg");
        #endif
        delay(10);//give some time between each message
      }
    }
  }
}

/* deal with Action messages */
boolean checkAction(){    
    if (car.isAction()){      
       if ( car.getNodeNumber() == id ){
          
             uint8_t action=car.getAction();
             uint8_t e, p, v, v1;
             int aux, aux1;
  /*           
           if (action == AC_ACQUIRE){
              if (acquired){
                 //uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t action,uint8_t val0,uint8_t val1);
                 car.sendAddressedActionMessage(rc, id,
                                            BOARD, AC_FAIL, 
                                            highByte(acquire_server), lowByte(acquire_server));
              }
              else{
                acquire_server = word(car.getVal0(), car.getVal1());
                acquired = true;
                car.sendAddressedActionMessage(rc, id, BOARD, AC_ACK, 0, 0);
                steering.write(midang);
                steering.attach(STEERING_PIN);
              }
          } 
          else if (action == AC_RELEASE){
              if (acquired){
                acquired = false;
                car.sendAddressedActionMessage(rc, id, BOARD, AC_ACK, 0, 0);
              }
          } */         
          
          if (action == AC_MOVE){   
                         
              v = car.getVal0();  
              v1 = car.getVal1();                               
              if (e == MOTOR || e == 0) {
                  //uint8_t d = car.getVal1();
                  if (v1 == 0){
                    motorpin = MOTOR_PIN;
                  }
                  else{
                    motorpin = MOTOR_PIN1;
                  }
                  if (v1 == lastspeed){                    
                    SoftPWMSetPercent(motorpin,v);                     
                  }
                  else {
                    if (v1 == 0){
                      SoftPWMSetPercent(motorpin,0);                      
                    }
                    else{
                      SoftPWMSetPercent(motorpin,0);                      
                    }
                    lastspeed = v1;
                    SoftPWMSetPercent(motorpin,v);
                    if (motorpin == MOTOR_PIN){
                      SoftPWMSetPercent(MOTOR_PIN1,0);                      
                    }
                    else{
                      SoftPWMSetPercent(MOTOR_PIN,0);
                    }
                  }                                   
              }
              return true;
          }
          else if (action == AC_TURN){   
              
              v = car.getVal0();
              v1 = car.getVal1();
              //direction
              if (v1 == 0){
                if (lastAng != (midang + v + boardParams[v1])){                  
                  steering.write(midang + v + boardParams[v1]);
                  lastAng = midang + v + boardParams[v1];                  
                  //delay(10);
                }                
              }
              else if (v1 == 1) {
                  if (lastAng != (midang - v - boardParams[v1])){                    
                    steering.write(midang - v - boardParams[v1]);
                    lastAng = midang - v - boardParams[v1];                    
                    //delay(10);
                  }
              }
              return true;              
          }
       }
    }
    return false;
}


void checkBattery(){  
    dvalues[0] = analogRead(BATTERY_PIN);
    uint8_t bperc = (uint8_t)(100*((BAT_LEVEL_READ - dvalues[0])/(float)(BAT_LEVEL_READ - BAT_FULL_LEVEL)));    
    if (bperc < 20){
      //send message
      if (bat_send_timer < (millis() + 5000 )){ //wait 5 sec to send again
        car.sendLowBattery(rc,id);
        #ifdef DEBUG_CAR
        Serial.println("send low bat");
        #endif
        bat_send_timer = millis();
      }      
    }     
}


