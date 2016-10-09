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

#define NUM_CARS    20
#define NUM_SERVERS 10
#define REG_TIMEOUT 2000 //2 secs
#define RC_TIMEOUT  5000 //5 secs

byte cars[NUM_CARS];
byte servers[NUM_SERVERS];
byte numrcs = 0;

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

byte motorpin = MOTOR_PIN;
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
 byte boardParams[5];

//timers
// auto enunmeration 
long t1,t2;
// send RC registration
long t3;
// last rc message
long t4;
int t;
long refresh_registration;
long last_registration;
byte LAST_MESSAGE_TIMEOUT = 30; //30 seconds
double long last_message = 0;
/*
 * battery vars
 */
long bat_send_timer = 0; //time between lowbat level

byte id = 0;
bool resolved = false;
bool sentreg = false;
bool rc_connected = false;
byte i = 0;


/*
 * steering variable
 * and call back function passed to softPWM
 * to refresh the soft servo lib
 */
SoftwareServo myservo;
byte midang = 92;
byte lastAng = 0;
byte lastspeed = 0;

boolean acquired;
bool newMessage;
byte rc;
int acquire_server; //TODO: check if we need it

void refreshSoftServo(int a){
  SoftwareServo::refresh();
}


void setup(){
  #ifdef DEBUG_CAR
  Serial.begin(115200);
  Serial.setTimeout(500);
  #endif
  
  /*
  * Start the radio
  * try 10 times if failed
  */
  boolean r=false;
  for (byte a=0;a<10;a++){
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
  driver.setModemConfig(RH_RF69::FSK_Rb250Fd250);
  randomSeed(analogRead(apin));
  id = EEPROM.read(0);
  if (id != EEPROM.read(1)){
    //get a random value if the old one is not the same
    id = random(1 , 255);
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

    /*
   * Start steering
   */
  myservo.write(midang);
  myservo.attach(STEERING_PIN);
  SoftPWMBegin(SOFTPWM_NORMAL,&refreshSoftServo);

  refresh_registration = random(200, 5000);
  
}

void loop(){
  
  if (!resolved) resolveId();

  if (resolved){
    sendCarRegistration();
  }

  newMessage = car.readMessage();
  if (newMessage){
    checkCarAutoEnum();
 
    if (car.isAcquire() && car.getId() == id){
        //check if server is registered
        rc = car.getServerId();
        for ( i = 0; i < NUM_SERVERS; i++){
          if ( servers[i] == rc){
            acquired = true;
            break;
          }
        }
    }
    if (acquired) {
      if (car.isCarRelease() && car.getId() == id){
        SoftPWMSetPercent(MOTOR_PIN, 0);
        SoftPWMSetPercent(MOTOR_PIN1, 0);
        acquired = false;
      }
      
      checkAction();     
      //checkQuery();
      renewConnected();
    }
    else{
      myservo.write(midang);      
    }
  }

  //dvalues[1] = analogRead(MOTOR_ROTATION_PIN);    
  //checkBattery(); 
}

void resolveId(){
  /*send autoenum message if not registered*/
  
  if (millis() - t1 > t){
    t1 = millis();
    car.sendCarAutoEnum(id);
  }
  if (!sentreg){
    t2 = millis();
    sentreg = true;
  }  

  if (millis() - t2 > REG_TIMEOUT){
    //end timer. transverse the data until find a valid id
    boolean f = true;
    
    #ifdef DEBUG_CAR
    Serial.println ("resolving the id");
    #endif
    
    while (f){
      f = false;
      for (byte j = 0; j < i; j++){
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
      Serial.print ("sent my id: ");Serial.println(id);
      #endif
      
    }

    if (car.isRCId()){
      bool rc_exist = false;
      byte rc = car.getId();
      for (i=0; i < NUM_SERVERS; i++){
        if (servers[i] == rc){
          rc_exist = true;
          break;
        }
      }
      if (!rc_exist){
        servers[numrcs] = rc;
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
    if (!rc_connected){
      for (byte j=0;j<numrcs;j++){
        car.sendInitialRegisterMessage(servers[j], id, ACTIVE, 0, 0, 0);
        delay(5);//give some time between each message
      }
    }
  }
}

void renewConnected(){
  /* if connected to an RC. 2 seconds without any message means disconnected*/
  if (rc_connected){
    if (millis() - t4 > 2000) {
      rc_connected = false;
    }
  }
}

/* deal with Action messages */
void checkAction(){    
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
                myservo.write(midang);
                myservo.attach(STEERING_PIN);
              }
          } 
          else if (action == AC_RELEASE){
              if (acquired){
                acquired = false;
                car.sendAddressedActionMessage(rc, id, BOARD, AC_ACK, 0, 0);
              }
          }
 */         
          if (action == AC_MOVE){              
              v = car.getVal0();  
              v1 = car.getVal1();                               
              if (e == MOTOR || e == 0) {
                  //byte d = car.getVal1();
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
                  }                                   
              }
          }
          else if (action == AC_TURN){              
              v = car.getVal0();
              v1 = car.getVal1();
              //direction
              if (v1 == 0){
                if (lastAng != (midang + v + boardParams[v1])){
                  //myservo.attach(STEERING_PIN);
                  //delay(10);
                  myservo.write(midang + v + boardParams[v1]);
                  lastAng = midang + v + boardParams[v1];
                  //delay(20);                           
                  //myservo.detach();
                }                
              }
              else {
                  if (lastAng != (midang - v - boardParams[v1])){
                    //myservo.attach(STEERING_PIN);
                    //delay(10);
                    myservo.write(midang - v - boardParams[v1]);
                    lastAng = midang - v - boardParams[v1];
                    //delay(20);                            
                    //myservo.detach();
                  }
              }    
              
          }
       }
    }
}


void checkBattery(){  
    dvalues[0] = analogRead(BATTERY_PIN);
    byte bperc = (byte)(100*((BAT_LEVEL_READ - dvalues[0])/(float)(BAT_LEVEL_READ - BAT_FULL_LEVEL)));    
    if (bperc < 20){
      //send message
      if (bat_send_timer < (millis() + 5000 )){ //wait 5 sec to send again
        car.sendLowBattery(rc,id);
        bat_send_timer = millis();
      }      
    }     
}


