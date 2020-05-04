/*
* This code is for the remote control car with remote controller
*/
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

#define NUM_CARS    10    //number of cars for the auto enum
#define NUM_SERVERS 5     //number of RCs registered
#define REG_TIMEOUT 2000 //2 secs. Auto enum timeout
#define EPROM_ID_ADDR 0
#define EPROM_OLD_ID_ADDR 1
#define EPROM_MID_ANGLE_ADDR 2
#define EPROM_MAX_RIGHT_ANGLE_ADDR 3
#define EPROM_MAX_LEFT_ANGLE_ADDR 4

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
#define CAR_KEEP_ALIVE_TIME       1000 //ms
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
//actual time
unsigned long act = 0;
//random time value used to address resolution
unsigned long t;
//generic multi purpose counter
long counter = 0;
//car timeout
long last_car_keepalive = 0;
/*
 * battery vars
 */
long bat_send_timer = 0; //time between lowbat level
//resolved car id
uint8_t id = 0;
//flag that indicates the address was resolved
bool resolved = false;
//flag indicating the registration was sent
bool sentreg = false;
//generic aux variable
uint8_t i = 0;


/*
 * steering variable
 * and call back function passed to softPWM
 * to refresh the soft servo lib
 */
SoftwareServo steering;
//middle angle of the servo when steering. can be dynamically adjusted
uint8_t midang = 90;
uint8_t max_angle_right = 10; // the max variation over the midangle. can be changed by configuration
uint8_t max_angle_left = 10; // the max variation over the midangle. can be changed by configuration
//last andgle set. used to avoid repeate setting
uint8_t lastAng = 0;
//last speed set. used to avoid repeate setting
uint8_t lastspeed = 0;
//variable indicating the car is being controlled by one rc
volatile boolean acquired = false;
//flag indicating a new message was received
bool newMessage;
//the remote control id
uint8_t rc;

//each 20ms
//used in the timer to refresh the soft servo
//the servo lib is not used because it clashes with the radio and softpwm
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
  //give time to the servo to move
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
    Serial.println(F("FAILED"));
    #endif
  }
  driver.setTxPower(13);
  driver.setModemConfig(RH_RF69::FSK_Rb250Fd250);

  /*read the id*/
  randomSeed(analogRead(apin));
  id = EEPROM.read(EPROM_ID_ADDR);
  if (id != EEPROM.read(EPROM_OLD_ID_ADDR)){
    //get a random value if the old one is not the same
    id = random(1 , 255);
  }

  /*get the middle angle*/
  midang = EEPROM.read(EPROM_MID_ANGLE_ADDR);
  if (midang < 80 || midang > 100){
    midang = 90; 
  }
  steering.write(midang);  
  SoftwareServo::refresh();
  
  /* get the max angle for steering */
  max_angle_right = EEPROM.read(EPROM_MAX_RIGHT_ANGLE_ADDR);
  if (max_angle_right > 15 || max_angle_right == 0){
      max_angle_right = 10;
  }
  max_angle_left = EEPROM.read(EPROM_MAX_LEFT_ANGLE_ADDR);
  if (max_angle_left > 15 || max_angle_left == 0){
      max_angle_left = 10;
  }

  t1 = millis();  
  t3 = t1 - RC_TIMEOUT;//just to guarantee we send it as fast as we can

  #ifdef DEBUG_CAR
  Serial.print(F("random id: "));Serial.println(id);
  #endif

  t = random(0, 2000);

  //retry timer
  #ifdef DEBUG_CAR
  Serial.print(F("random retry timer: "));Serial.println(t);
  #endif

   /*
   * charger pin and reed pin 
   */
  pinMode(CHARGER_PIN, INPUT);  
  pinMode(BATTERY_PIN, INPUT);
  pinMode(MOTOR_ROTATION_PIN, INPUT);  
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
    
  //debuging thing
  #ifdef DEBUG_CAR
    printMessageType(car.getByte(0));
    //if (car.isRCKeepAlive()){     
    //  Serial.print(F("kpa "));Serial.print(car.getId());Serial.print(F(" "));Serial.println(car.getServerId());    
    //}
  #endif
    //do auto enum on start
    checkCarAutoEnum();
    //check if some rc send acquire
    checkAcquire();

    //avoid non necessary work 
    if (isForMe()){
      
      #ifdef DEBUG_CAR
      Serial.println(F("for me"));
      #endif
      
      if (acquired) {
        SoftwareServo::refresh();  
          //check the keep alive from the rc
          checkKeepAlive();
          
          #ifdef DEBUG_CAR
          //Serial.print(F("sp "));Serial.println(analogRead(MOTOR_ROTATION_PIN));
          #endif
          //check messages to move the car          
          if (checkMove()) tk_rc = act; 
          //check messages to turn left or right
          if (checkTurn()) tk_rc = act; 
          //check message to stop the car
          if (checkStopCar()) tk_rc = act;          
          //check message to save parameters
          checkSaveParam();
          SoftwareServo::refresh();         
      }   
    } 
  }

  if (acquired){
    SoftwareServo::refresh();
    //check the timeout for not receiving a keep alive from the RC
    checkKeepAliveTimeout();
    checkRelease();
    
    if ((act - last_car_keepalive) >= CAR_KEEP_ALIVE_TIME){
      car.sendCarKeepAlive(id, rc);
      last_car_keepalive = act;
      #ifdef DEBUG_CAR
        Serial.println(F("send keep alive"));
      #endif
    }
  }
  else{
    steering.detach();
  }
   //checkBattery();
  //dvalues[1] = analogRead(MOTOR_ROTATION_PIN);    
   
}

bool isForMe(){  
   #ifdef DEBUG_CAR
    Serial.print(F("nodeid "));Serial.println(car.getNodeId());
    Serial.print(F("id "));Serial.println(id);
    Serial.print(F("rc "));Serial.println(rc);
    Serial.print(F("serverid "));Serial.println(car.getServerId());   
   #endif
  return ((car.getNodeId() == id) && (rc == car.getServerId()));
}

void checkSaveParam(){
  if (car.isSaveParam() && isForMe()){
        #ifdef DEBUG_CAR
        Serial.print(F("trimming "));
        Serial.println(car.getParamIdx());
        #endif
        uint8_t pidx = car.getParamIdx(); 
        tk_rc = act;
        if ( pidx == 1){
          // middle angle
          midang = car.getVal0();
          #ifdef DEBUG_CAR
          Serial.print(F("trimming val "));Serial.println(midang);
          #endif
          if (midang >= 80 && midang <=100){
            steering.write(midang);  
          }
          else midang = 90;
          EEPROM.write(EPROM_MID_ANGLE_ADDR,midang);          
        }
        else if (pidx == 2){
        //max angle
          max_angle_right = car.getVal0();
          if (max_angle_right > 15) max_angle_right = 15;        

          EEPROM.write(EPROM_MAX_RIGHT_ANGLE_ADDR,max_angle_right);          
        }
        else if (pidx == 3){
        //max angle
          max_angle_left = car.getVal0();
          if (max_angle_left > 15) max_angle_left = 15;        

          EEPROM.write(EPROM_MAX_LEFT_ANGLE_ADDR,max_angle_left);          
        }
  }
}

void checkRelease(){
  
  if (car.isCarRelease() && isForMe()){
    #ifdef DEBUG_CAR
    Serial.println(F("rec release"));
    #endif
    SoftPWMSetPercent(MOTOR_PIN, 0);
    SoftPWMSetPercent(MOTOR_PIN1, 0);
    car.sendCarReleaseAck(id, rc);
    delay(50);
    car.sendCarReleaseAck(id, rc);
    #ifdef DEBUG_CAR
    Serial.println(F("send release ack"));
    #endif
    acquired = false;
  }
}


void checkKeepAliveTimeout(){
   
  if ((act - tk_rc) > RC_TIMEOUT){
    #ifdef DEBUG_CAR
    Serial.println(F("timeout"));
    Serial.print(F("act "));Serial.println(act);
    Serial.print(F("tkrc "));Serial.println(tk_rc);
    Serial.print(F("diff "));Serial.println(act - tk_rc);
    Serial.print(F("RC_TIMEOUT "));Serial.println(RC_TIMEOUT);
    #endif
    SoftPWMSetPercent(MOTOR_PIN, 0);
    SoftPWMSetPercent(MOTOR_PIN1, 0);
    car.sendCarReleaseAck(id, rc);
    delay(50);
    car.sendCarReleaseAck(id, rc);
    #ifdef DEBUG_CAR
    Serial.println(F("send release ack"));
    #endif
    acquired = false;
  }
}

void checkKeepAlive(){
  if (car.isRCKeepAlive()){
      #ifdef DEBUG_CAR
      Serial.println(F("rec keep alive"));
      #endif
     if (isForMe()){
  
       tk_rc = act; //renew the last keep alive
       #ifdef DEBUG_CAR
      Serial.println(F("keep alive for me"));
      #endif
     }
  }
}

void checkAcquire(){
  if (car.isAcquire() && car.getNodeId() == id && !acquired){
        //check if server is registered
        #ifdef DEBUG_CAR
        Serial.print(F("rec acquire numserv "));Serial.println(car.getServerId());
        #endif
        
        rc = car.getServerId();        
        for ( i = 0; i < NUM_SERVERS; i++){          
          if ( servers[i] == rc){            
            acquired = true;
            car.sendAcquireAck(id, rc);            
            #ifdef DEBUG_CAR
            Serial.println(F("send acquire ack"));
            #endif            
            steering.write(midang);
            steering.attach(STEERING_PIN);
            tk_rc = act;            
            break;
          }
        }
    }
}

bool checkStopCar() {
  if (car.isStopCar() && isForMe()){
        #ifdef DEBUG_CAR
        Serial.println(F("stopping car"));
        #endif
        SoftPWMSetPercent(MOTOR_PIN, 0);
        SoftPWMSetPercent(MOTOR_PIN1, 0);
        return true;
  }
  return false;
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
      Serial.println(F("send auto enum"));
      #endif
    }
  }    

  if (millis() - t2 > REG_TIMEOUT){
    //end timer. transverse the data until find a valid id
    boolean f = true;
    
    #ifdef DEBUG_CAR
    Serial.println ("resolving the id"));
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
    Serial.print ("valid id: "));Serial.println(id);
    #endif
    
    resolved = true;
    //save to eprom
    //save both value in the 2 first positions, so we can assure we did it
    EEPROM.write(EPROM_ID_ADDR,id);
    EEPROM.write(EPROM_OLD_ID_ADDR,id);
  }
}
void checkCarAutoEnum(){
    
    if (car.isCarId() && !resolved){
      cars[i] = car.getNodeId();

      #ifdef DEBUG_CAR
      Serial.print ("received id: "));Serial.println(servers[i]);
      #endif
      
      i++;
      if (i > NUM_CARS) i = NUM_CARS -1;
    }
    
    if (car.isCarAutoEnum()){      
      car.sendCarId(id);
      #ifdef DEBUG_CAR
      Serial.println(F("send id for autoenum"));
      #endif
      
      #ifdef DEBUG_CAR
      Serial.print(F("sent my id: "));Serial.println(id);
      #endif
      
    }

    if (car.isRCId()){
      bool rc_exist = false;
      uint8_t rc = car.getNodeId();

      #ifdef DEBUG_CAR
      Serial.print(F("rc rec "));Serial.println(rc);
      #endif
      
      for (i=0; i < NUM_SERVERS; i++){
        if (servers[i] == rc){
          rc_exist = true;
          #ifdef DEBUG_CAR
          Serial.println(F("rc exist"));
          #endif
          if (numrcs == 0) numrcs++;
          break;
        }
      }
      if (!rc_exist){
        servers[numrcs] = rc;
        #ifdef DEBUG_CAR
          Serial.print(F("rc registered "));Serial.println(numrcs);
        #endif
        numrcs ++;
        if (numrcs >= NUM_SERVERS) {
          numrcs = NUM_SERVERS - 1;
        }
      }
    }
}

void sendCarRegistration(){

  if (acquired) return;
  
  if (millis() - t3 > RC_TIMEOUT){
    
    t3 = millis();
    if (!acquired){
      #ifdef DEBUG_CAR
        Serial.println(F("try send reg"));
      #endif
      for (uint8_t j=0;j < numrcs; j++){
        //car.sendInitialRegisterMessage(servers[j], id, ACTIVE, 0, 0, 0);
        car.sendRCCarRegister(id, servers[j]);
        #ifdef DEBUG_CAR
        Serial.println(F("send reg"));
        #endif
        delay(10);//give some time between each message
      }
    }
  }
}

bool checkMove(){
  if (car.isRCMove() && isForMe()){
    uint8_t p_speed;
    uint8_t p_dir;
    p_speed = car.getSpeed();  
    p_dir = car.getDirection();
      
      if (p_dir == 0){
        motorpin = MOTOR_PIN;
      }
      else{
        motorpin = MOTOR_PIN1;
      }
      #ifdef DEBUG_CAR
      Serial.print(F("speed "));Serial.println(p_speed);
      #endif
      if (p_dir == lastspeed){                    
        SoftPWMSetPercent(motorpin,p_speed);                     
      }
      else {
        if (p_dir == 0){
          SoftPWMSetPercent(motorpin,0);                      
        }
        else{
          SoftPWMSetPercent(motorpin,0);                      
        }
        lastspeed = p_dir;
        SoftPWMSetPercent(motorpin,p_speed);
        if (motorpin == MOTOR_PIN){
          SoftPWMSetPercent(MOTOR_PIN1,0);                      
        }
        else{
          SoftPWMSetPercent(MOTOR_PIN,0);
        }
      }                                   
    
    return true;
  }
  return false;
}

boolean checkTurn(){
  if (car.isRCTurn() && isForMe()){
    uint8_t p_angle;
    uint8_t p_dir;
    uint8_t ang;

    p_angle = car.getAngle();  
    p_dir = car.getDirection();

    /*
    * the server sends a parcentage of the movement
    * we transform it an angle
    */
    if (p_dir == 0){
      ang = map (p_angle, 0, 100, 0, max_angle_right);
    }
    else{
      ang = map (p_angle, 0, 100, 0, max_angle_left);
    }
    
    
    //direction
    if (p_dir == 0){
      if (lastAng != (midang + ang )){                  
        steering.write(midang + ang );
        lastAng = midang + ang ;        
      }                
    }
    else if (p_dir == 1) {
        if (lastAng != (midang - ang )){                    
          steering.write(midang - ang );
          lastAng = midang - ang ;          
        }
    }
    return true; 
  }
  return false;
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
                  #ifdef DEBUG_CAR
                  Serial.print(F("speed "));Serial.println(v);
                  #endif
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
                if (lastAng != (midang + v )){                  
                  steering.write(midang + v );
                  lastAng = midang + v ;                  
                  //delay(10);
                }                
              }
              else if (v1 == 1) {
                  if (lastAng != (midang - v )){                    
                    steering.write(midang - v );
                    lastAng = midang - v ;                    
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
        Serial.println(F("send low bat"));
        #endif
        bat_send_timer = millis();
      }      
    }     
}

void printMessageType(uint8_t msgtype){
  Serial.print(F("message type "));
  switch (msgtype){
    case(RP_BROADCAST):
      Serial.println(F("RP_BROADCAST"));
      break;
    case(RP_ADDRESSED):
      Serial.println(F("RP_ADDRESSED"));
      break;
    case(RP_STATUS):
      Serial.println(F("RP_STATUS"));
      break;
    case(RP_ID_RESOLUTION):
      Serial.println(F("RP_ID_RESOLUTION"));
      break;
    case(BR_SERVER_AUTO_ENUM):
      Serial.println(F("BR_SERVER_AUTO_ENUM"));
      break;
    case(BR_CAR_AUTO_ENUM):
      Serial.println(F("BR_CAR_AUTO_ENUM"));
      break;
    case(RC_ID):
      Serial.println(F("RC_ID"));
      break;
    case(CAR_ID):
      Serial.println(F("CAR_ID"));
      break;
    case(CAR_ACQUIRE):
      Serial.println(F("CAR_ACQUIRE"));
      break;
    case(CAR_RELEASE):
      Serial.println(F("CAR_RELEASE"));
      break;
    case(CAR_ACQUIRE_ACK):
      Serial.println(F("CAR_ACQUIRE_ACK"));
      break;
    case(CAR_RELEASE_ACK):
      Serial.println(F("CAR_RELEASE_ACK"));
      break;
    case(CAR_KEEP_ALIVE):
      Serial.println(F("CAR_KEEP_ALIVE"));
      break;
    case(RC_KEEP_ALIVE):
      Serial.println(F("RC_KEEP_ALIVE"));
      break;
    case(SAVE_PARAM):
      Serial.println(F("SAVE_PARAM"));
      break;
    case(RC_LIGHTS):
      Serial.println(F("RC_LIGHTS"));
      break;
    case(RC_BREAK_LIGHTS):
      Serial.println(F("RC_BREAK_LIGHTS"));
      break;
    case(RC_STOP_CAR):
      Serial.println(F("RC_STOP_CAR"));
      break;
    case(RC_MOVE):
      Serial.println(F("RC_MOVE"));
      break;
    case(RC_TURN):
      Serial.println(F("RC_TURN"));
      break;
    case(CAR_ACQUIRE_NACK):
      Serial.println(F("CAR_ACQUIRE_NACK"));
      break;
    case(RC_CAR_REGISTER):
      Serial.println(F("RC_CAR_REGISTER"));
      break;
    case(RC_CAR_REGISTER_ACK):
      Serial.println(F("RC_CAR_REGISTER_ACK"));
      break;           
  }
  
}

