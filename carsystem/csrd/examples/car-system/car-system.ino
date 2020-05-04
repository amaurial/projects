/*
 The code implements a machine state with states
 ACTIVE=0,
 INACTIVE,
 CHARGING,
 PANNE,
 REGISTERED,
 NOT_REGISTERED,
 WAITING_REGISTRATION
 The firs state is NOT_REGISTERED
 NOT_REGISTERED -> triggers sendRegistrationMessage -> WAITING_REGISTRATION
 WAITING_REGISTRATION -> triggers sendRegistrationMessage if timeout, confirmRegistrationMessage
 if registration is confirmed state -> ACTIVE

*/

#include "Arduino.h"

#include <csrd.h>
#include <RHReliableDatagram.h>
#include <RH_RF69.h>
#include <SPI.h>
#include <EEPROM.h>
#include <SoftPWM.h>

// include the remote library just if we are using mega328pb because of the extra timer
// change here to include the mega32u4
#if defined(__AVR_ATmega328PB__)
#include <IRremote.h>
IRsend irsend;
#endif

//uncomment for debug message
#define DEBUG_CAR 1;

//PINS
#define FRONT_LIGHT_PIN           7//A1
#define LEFT_LIGHT_PIN            5
#define RIGHT_LIGHT_PIN           4
#define SIRENE_LIGHT_PIN          A3 //AUX2
#define BREAK_LIGHT_PIN           6//aux1
#define REED_PIN                  A2//AUX4
#define MOTOR_PIN                 A4
#define MOTOR_ROTATION_PIN        A6//A3
#define BATTERY_PIN               A7//A5
#define IR_RECEIVE_PIN            A0 //AUX3
#define IR_SEND_PIN               3 //AUX1 IR Remote just accepts the port d3
#define CHARGER_PIN               A1

#define BAT_FULL_LEVEL            610 //analog read equivalent to 4V
#define BAT_LEVEL_READ            680 //by tests 688 the motor stops

#define MAXPARAMS                 7 //Maximum number of parameters for each element

#define MEMORY_REF                23 //start of the memory
#define EPROM_SIZE                512
#define IR_WAIT_TIME              10 //this value times the IR parameter
#define IR_READ_DELAY             500 // microsends

//holds the configuration and dynamic data of each element: motor and lights
typedef struct ELEMENTS *ELEMENT; 

//pointer to the functions that will control the parts
//actual state,next state, parameters,type,num parameters
typedef void(*controllers)(ELEMENT);

//structure that holds all important data
typedef struct ELEMENTS {
  states state;      //the current element state
  states next;       //the current element state
  states tempState;  //temporary element state
  states lastState;  //the last element state
  states auxState;
  objects_enum obj;
  byte params[MAXPARAMS];
  byte total_params;
  controllers controller;
  long auxTime;
  byte port;          //the hardware port
  byte actual_pwm_val;
};
//number of elements
#define NUM_ELEMENTS 9    // the elements: motor, lights, ir sensor, reed sensor
struct ELEMENTS elements[NUM_ELEMENTS];

//dynamic values
//[0] = batery
//[1] = motor
#define D_VALUES 5
uint16_t dvalues[D_VALUES];

//radio buffer
byte radioBuffer[RH_RF69_MAX_MESSAGE_LEN];
RH_RF69 driver; // pin 8 for mega32u4, empty for mega328
//RHReliableDatagram manager(driver, 33);

//car message lib
CSRD car;

//aux vars
long actime;
long st;
long count;
float motor_rotation;
int msamples;

//ir variables to keep the cars distance
float irAux;
long irSum;
int force;
int distance;

//battery vars
unsigned long bat_send_timer = 0; //time between lowbat level

//incomming and outgoing buffer
uint8_t sbuffer[MESSAGE_SIZE];
uint8_t recbuffer[MESSAGE_SIZE];

//node status
STATUS status;

//node number and group
uint16_t nodeid = 999;     // each car has a node_id
uint8_t group = 1;         // the cars can be organized as group
uint8_t serverStation = 1; // and can be several server stations

//message flag
bool newMessage = false;

// time variables
// every car sends a registration from time to time
long refresh_registration;
long last_registration;

// registration random intervals in millis
#define REG_RANDOM_INITIAL_1 100
#define REG_RANDOM_END_1 3000
#define REG_RANDOM_INITIAL_2 200
#define REG_RANDOM_END_2 5000

//functions to control the elements
//void controlBreakLeds(ELEMENTS * element);
void controlBlinkLed(ELEMENTS * element);
void controlMotor(ELEMENTS * element);
void controlAux(ELEMENTS * element);
void controlReed(ELEMENTS * element);
void controlIRSend(ELEMENTS * element);
void controlIRReceive(ELEMENTS * element);

/*
Print nice status messages
*/
void printStatus() {

  #ifdef DEBUG_CAR 

  switch (status) {
    case WAITING_REGISTRATION:
      Serial.println(F("status WAITING_REGISTRATION"));
      break;
    case ACTIVE:
      Serial.println(F("status ACTIVE"));
      break;
    case NOT_REGISTERED:
      Serial.println(F("status NOT_REGISTERED"));
      break;
    case INACTIVE:
      Serial.println(F("status INACTIVE"));
      break;
    case CHARGING:
      Serial.println(F("status CHARGING"));
      break;
    case PANNE:
      Serial.println(F("status PANNE"));
      break;
    case REGISTERED:
      Serial.println(F("status REGISTERED"));
      break;
  }

  #endif
}

void setup() {
  
  #ifdef DEBUG_CAR
    Serial.begin(19200);
    while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only    
    Serial.print(F("CPU freq: "));
    Serial.println(F_CPU);
  }
  #endif
  
  //charger pin
  pinMode(CHARGER_PIN, INPUT);
  pinMode(IR_SEND_PIN, OUTPUT);
  //reed pin
  pinMode(REED_PIN, INPUT_PULLUP);

  
  //library that controls the motor
  SoftPWMBegin();
  // config the radio
  startRadio();
  
  count = 0;
  status = NOT_REGISTERED;

  uint16_t n = getNodeIdFromEprom();
  if (n > 4095){ //x0FFF
    // not initialized, then also set default params
    #ifdef DEBUG_CAR
    Serial.println(F("Initializing nodeid 999 and setting default parameters"));
    #endif
    nodeid = 999;
    saveNodeIdToEprom(nodeid);
    setDefaultParams();
  }
  else{
    nodeid = n;
  }
  //setDefaultParams();
  initElements();
  randomSeed(analogRead(A0));
  refresh_registration = random( REG_RANDOM_INITIAL_2, REG_RANDOM_END_2 );
  #ifdef DEBUG_CAR
    Serial.println(F("START CLIENT"));
  #endif
}

void startRadio(){
//if (!car.init(&driver,&manager)){ //this is sort of tcp library
  // initialize the library
  boolean radion_on = false;
  for ( byte a = 0; a < 10; a++ ){
    if ( car.init(&driver, NULL )) {
        radion_on = true;
        break;
    }
      delay(200);
   }
   
   if ( radion_on == false ) {
     #ifdef DEBUG_CAR
      Serial.println(F("FAILED THE RADIO"));
    #endif
    //turn on the sirene to blink indicating failure
    elements[SIRENE_LIGHT].next = BLINKING;
  }
  
  driver.setModemConfig(RH_RF69::GFSK_Rb250Fd250);
  driver.setFrequency(915.00); // should this be a configuration?
  uint8_t syncwords[2];
  syncwords[0] = 0x10;
  syncwords[1] = 0x10;; //(uint8_t)group;   
  driver.setSyncWords(syncwords, sizeof(syncwords));
  driver.setEncryptionKey(NULL);
  driver.setPromiscuous(true);
    
  // If you are using a high power RF69, you *must* set a Tx power in the
  // range 14 to 20 like this:
  driver.setTxPower(15); // should this be a configuration?
}

void loop() {            
    actime = millis();  
        
    //carry the active actions
    //if (status == ACTIVE) {
      for (byte i = 0; i < NUM_ELEMENTS; i++) {
        elements[i].controller(&elements[i]);
      }
    //} 

    newMessage = car.readMessage();
  
    if ( newMessage ) {
      #ifdef DEBUG_CAR
        dumpMessage();
      #endif
      confirmRegistrationMessage();
      checkBroadcastRegister();
    }
 
    //send registration message
    sendRegistrationMessage();
    
    // answer requests or execute commands
    if (newMessage){      
      checkMsgRestoreDefault();        
      checkMsgWriteParameter();
      checkAction();
      checkQuery();
      checkQueryState();
      
      if (status == ACTIVE){
        setNextOperation();
      }
    }    
    //msamples++;
    //motor_rotation = (motor_rotation + analogRead(MOTOR_ROTATION_PIN))/2.0;
    dvalues[1] = analogRead(MOTOR_ROTATION_PIN);    
    checkBattery();   
}

/*
*/

void dumpMessage() {

  #ifdef DEBUG_CAR  
    Serial.println(F("New Message"));
    car.getMessageBuffer(recbuffer);
    for (byte i = 0; i < MESSAGE_SIZE; i++) {
      Serial.print(recbuffer[i]);
      Serial.print(F("   "));
    }
    Serial.println();
  #endif
}

//restore default parameters
void checkMsgRestoreDefault(){  
    if (car.isRestoreDefaultConfig(nodeid)){
       #ifdef DEBUG_CAR
            Serial.println(F("Restore default values"));
      #endif
       setDefaultParams();
       initElements();
    }
}

//confirm we received a registration
void confirmRegistrationMessage(){
  if (status == WAITING_REGISTRATION) {      
    //this is registration return message
    if (car.isStatus() && car.getNodeNumber() == nodeid && car.getStatus() == ACTIVE) {
      status = ACTIVE;
      #ifdef DEBUG_CAR
            Serial.println(F("STATUS ACTIVE"));
      #endif
    }
  }
}

//check broadcast register
// the stations sends register from time to time
// this is a way to force re registrations
void checkBroadcastRegister(){
  if (car.isBroadcastRegister() && car.isMyGroup(group) ){   
     #ifdef DEBUG_CAR
            Serial.println(F("BREG received status WAITING REG"));
      #endif
     status = WAITING_REGISTRATION;
      //set timer based on ramdom number to avoid all cars sending information at the same time
      randomSeed(analogRead(A0));
      refresh_registration = random(REG_RANDOM_INITIAL_1, REG_RANDOM_END_1);
      last_registration = millis();
  }
}

//send initial registration message
void sendRegistrationMessage(){
   
   if ( status == NOT_REGISTERED ||
       ( status == WAITING_REGISTRATION && (actime - last_registration) > refresh_registration) ) {
          
          #ifdef DEBUG_CAR
          /*
             Serial.println();
             Serial.println(F("Need to register?"));
             //printStatus();
             Serial.println(F("new message: "));
             Serial.println(newMessage);
             Serial.println(F("broadcast reg: "));
             Serial.println(car.isBroadcastRegister());
             Serial.println(F("is my group: "));
             Serial.println(car.isMyGroup(group));
             Serial.println(F("refresh registration time: "));
             Serial.print(actime - last_registration);
             Serial.println(F("\t"));
             Serial.println(refresh_registration);   
             Serial.println();
              Serial.println(F("Sending initial registration data"));
           */   
          #endif
      //send message. change the status
      #ifdef DEBUG_CAR
            Serial.println(F("send REG request"));
      #endif
      car.sendInitialRegisterMessage(serverStation, nodeid, ACTIVE, 0, 0, 0);
      status = WAITING_REGISTRATION;
      //set timer
      randomSeed(analogRead(A0));
      refresh_registration = random(REG_RANDOM_INITIAL_2, REG_RANDOM_END_2);
      last_registration = millis();
     
      #ifdef DEBUG_CAR
          //Serial.println(F("STATUS WAITING REGISTRATION"));
      #endif
  }
}

//deal with Action messages
void checkAction(){

  if ( !car.isAction()){ return; }

  if ( ! ((car.isAddressed() && (car.getNodeNumber() == nodeid)) ||
       (car.isBroadcast() && car.isMyGroup(group))) 
     ){ 
       return;
  }

  uint8_t action = car.getAction();
  if (action != AC_SET_PARAM){ return; }

  #ifdef DEBUG_CAR
    Serial.println(F("Processing action."));
  #endif

  uint8_t e,p,v;
  int aux, aux1;  
  
  e = car.getElement();
  p = car.getParamIdx();
  v = car.getVal0();                                    

  if (e != BOARD){
      if (p < elements[e].total_params){
        elements[e].params[p]=v;    
        if (p == 0){ //normally is the intensity (speed, light luminosity
            elements[e].actual_pwm_val = v;
        }
        if (e == MOTOR){
            aux = elements[e].params[1] * elements[e].params[2];
            aux1 = elements[e].params[1] * elements[e].params[3];                      
        }
        else{                      
            aux = elements[e].params[1] * elements[e].params[2];                      
            aux=aux1;
        }   
        setPWM(e,elements[e].actual_pwm_val,aux,aux1);  
        if ((e == MOTOR) && (elements[e].state == ON)) {    
            SoftPWMSetPercent(elements[e].port,elements[e].actual_pwm_val);
            return;
        }              
      }
  }
  else {
    //TODO: add board actions
  }  
    
}

//deal with query messages
void checkQuery(){  

  // ignore non status messages
  if ( !car.isStatus()){ return; }
  // ignore messages that are not for me
  if ( car.getNodeNumber() != nodeid ){ return; }
  // just consider status messages that requests value
  if ( car.getStatusType() != STT_QUERY_VALUE) { return; }

  #ifdef DEBUG_CAR
    Serial.println(F("Processing query."));
  #endif
  
  uint8_t e = car.getElement();
  uint8_t p0 = car.getVal0();
  uint8_t p1 = car.getVal1();
  uint8_t p2 = car.getVal2();
  
  switch (e){
    case BOARD:
        byte charger;
        charger = 0;
        // board has only 3 params: battery level, motor rotation, charger level                  
        // TODO: it is hard to understand. needs improvement
        if ((p0 < 3) && (p1 < 3) && (p2 < 3)){
          if (p2 == 2){                          
              if (digitalRead(CHARGER_PIN) == HIGH ){
                charger = 1;
              }
          }
          //0 -> battery 1->motor
          //4v -> 610 charged 3.3v -> 640 ; 0v ->1023
          //688 - motor stopped
          byte bperc = (byte)(100*((BAT_LEVEL_READ - dvalues[p0])/(float)(BAT_LEVEL_READ-BAT_FULL_LEVEL)));
          car.sendAddressedStatusMessage(STT_ANSWER_VALUE, car.getSender(), nodeid, e, bperc, dvalues[p1]*0.049, charger);  
        }
        else{
          car.sendAddressedStatusMessage(STT_QUERY_VALUE_FAIL, car.getSender(), nodeid, e, p0, p1, p2);  
        }                    
    break;
    default:
      {
      if (e >= NUM_ELEMENTS ){
          car.sendAddressedStatusMessage(STT_QUERY_VALUE_FAIL, car.getSender(), nodeid, e, 0, 0, 0);  
      }
      //sanity check on params
      uint8_t val0,val1,val2;
      if (p0 < elements[e].total_params){
        val0=elements[e].params[p0];
      }     
      else val0 = RP_FILLER;
      
      if (p1 < elements[e].total_params){
        val1 = elements[e].params[p1];
      }     
      else val1 = RP_FILLER;

      if (p2 < elements[e].total_params){
        val2 = elements[e].params[p2];
      }     
      else val2 = RP_FILLER;

      car.sendAddressedStatusMessage(STT_ANSWER_VALUE, car.getSender(), nodeid, e, val0, val1, val2);  
      }
  }               
}


//deal with Query state messages
void checkQueryState(){

  if ( !( car.isQueryState() || car.isQueryAllStates())){ return; }  
  // ignore messages that are not for me
  if ( car.getNodeNumber() != nodeid ){ return; }

  if (car.isQueryState()){
    uint8_t e;    
    e = car.getElement();
    #ifdef DEBUG_CAR
      Serial.println(F("Send query answer."));
    #endif
    car.sendAnswerState(car.getSender(), nodeid, e, elements[e].state);    
  }
  else{   
    #ifdef DEBUG_CAR   
      Serial.println(F("SQA LEFT_LIGHT"));
    #endif
    car.sendAnswerState(car.getSender(), nodeid, LEFT_LIGHT , elements[LEFT_LIGHT].state);     
    #ifdef DEBUG_CAR
      Serial.println(F("SQA RIGHT_LIGHT"));
    #endif
    car.sendAnswerState(car.getSender(), nodeid, RIGHT_LIGHT , elements[RIGHT_LIGHT].state);    
    #ifdef DEBUG_CAR
      Serial.println(F("SQA BREAK_LIGHT"));
    #endif
    car.sendAnswerState(car.getSender(), nodeid, BREAK_LIGHT , elements[BREAK_LIGHT].state);    
    #ifdef DEBUG_CAR
      Serial.println(F("SQA REED"));
    #endif
    car.sendAnswerState(car.getSender(), nodeid, REED , elements[REED].state);    
    #ifdef DEBUG_CAR
      Serial.println(F("SQA SIRENE_LIGHT"));
    #endif
    car.sendAnswerState(car.getSender(), nodeid, SIRENE_LIGHT , elements[SIRENE_LIGHT].state);    
    #ifdef DEBUG_CAR
      Serial.println(F("SQA FRONT_LIGHT"));
    #endif
    car.sendAnswerState(car.getSender(), nodeid, FRONT_LIGHT , elements[FRONT_LIGHT].state);    
    #ifdef DEBUG_CAR
      Serial.println(F("SQA MOTOR"));
    #endif
    car.sendAnswerState(car.getSender(), nodeid, MOTOR , elements[MOTOR].state);    
    #ifdef DEBUG_CAR
      Serial.println(F("SQA IR_RECEIVE"));
    #endif
    car.sendAnswerState(car.getSender(), nodeid, IR_RECEIVE , elements[IR_RECEIVE].state);    
    #ifdef DEBUG_CAR
      Serial.println(F("SQA IR_SEND"));
    #endif
    car.sendAnswerState(car.getSender(), nodeid, IR_SEND , elements[IR_SEND].state);    
    #ifdef DEBUG_CAR
      Serial.println(F("SQA BOARD"));
    #endif
    car.sendAnswerState(car.getSender(), nodeid, BOARD , elements[BOARD].state);    
  }
}

//deal with write message
void checkMsgWriteParameter(){  

  // ignore non write messages
  if ( !car.isWrite()){ return; }
  // ignore messages that are not for me
  if ( !( (car.isAddressed() ) || (car.isBroadcast() && car.isMyGroup(group)) ) ){ return; }
  
  uint8_t e = car.getElement();
  if ((e != BOARD) && (car.getNodeNumber() == nodeid)){
      
    uint8_t p = car.getParamIdx();
    uint8_t v = car.getVal0();
    int aux, aux1;
    
    if (e < NUM_ELEMENTS){
      if (p < elements[e].total_params){
          elements[e].params[p] = v;
          uint8_t result = saveParameterToEprom(elements[e].params , elements[e].total_params , e);
          
          if (p == 0){
            elements[e].actual_pwm_val = v;
          }
          if (e == MOTOR){
            aux = elements[e].params[1] * elements[e].params[2];
            aux1 = elements[e].params[1] * elements[e].params[3];                      
          }
          else{                      
            aux = elements[e].params[1] * elements[e].params[2];                      
            aux=aux1;
          }
          //send ack
          car.sendACKMessage(serverStation, nodeid, e, result);
          if (e != REED){//Reed has no fade
          
              SoftPWMSetFadeTime(elements[e].port, aux, aux1);
              
              if ((e == MOTOR) && (elements[e].state == ON)) {    
                Serial.println(F("Change motor speed"));
                SoftPWMSetPercent(elements[e].port, elements[e].actual_pwm_val);
                return;
              } 
              else{                 
                SoftPWMSet(elements[e].port, elements[e].actual_pwm_val);          
              }
          }
      }
    }             
  }
  else if (e == BOARD){
    uint8_t p = car.getParamIdx();
      if (p == 0){//Node id
        uint16_t n = word(car.getVal1(), car.getVal0());
        saveNodeIdToEprom(n);
        nodeid = n;
        status = NOT_REGISTERED;
        sendRegistrationMessage();
      }           
  }
}

//set next operation
void setNextOperation(){

    if (!(car.isOperation())) {
      return;  
    }  
  
    if (!(car.getNodeNumber() == nodeid || (car.isBroadcast() && car.isMyGroup(group) ))) {
       return;
    }

    //dumpParameters();
    
    byte e = car.getElement();
    states s = car.convertFromInt(car.getState());    
    if (e != BOARD) {
      if (e < NUM_ELEMENTS) {        
        elements[e].next = s;            
      }
    }
    else {      
      controlBoard(s);
    }      
}

void controlBoard(states s){
  switch (s) {
        case (EMERGENCY):
          //save actual state
          
          for (byte i = 0; i < NUM_ELEMENTS; i++) {
            elements[i].auxState = elements[i].state;
          }
          
          elements[LEFT_LIGHT].next = BLINKING;
          elements[RIGHT_LIGHT].next = BLINKING;
          elements[SIRENE_LIGHT].next = BLINKING;
          elements[BREAK_LIGHT].next = BLINKING;          
          elements[FRONT_LIGHT].next = BLINKING;
          elements[MOTOR].next = OFF;

          break;

        case (ALL_BLINKING):
          //save actual state
          
          for (byte i = 0; i < NUM_ELEMENTS; i++) {
            elements[i].auxState = elements[i].state;
          }
          elements[LEFT_LIGHT].next = BLINKING;
          elements[RIGHT_LIGHT].next = BLINKING;
          elements[SIRENE_LIGHT].next = BLINKING;
          elements[BREAK_LIGHT].next = BLINKING;          
          elements[FRONT_LIGHT].next = BLINKING;          

          break;

        case (NORMAL):
          //get last state
          //save actual state
          for (byte i = 0; i < NUM_ELEMENTS; i++) {
            elements[i].next = elements[i].auxState;
            //elements[i].tempState = OFF;
          }
          if (elements[MOTOR].next == STOPING) {
            elements[MOTOR].next = OFF;
          }
          if (elements[MOTOR].next == ACCELERATING) {
            elements[MOTOR].next = ON;
          }          
          break;

        case (NIGHT):        
          //Serial.println(F("night"));            
          elements[BREAK_LIGHT].next = ON;          
          elements[FRONT_LIGHT].next = ON;
          break; 

        case (DAY):           
          //Serial.println(F("day"));          
          elements[BREAK_LIGHT].next = OFF;          
          elements[FRONT_LIGHT].next = OFF;
          break; 

        case (OFF):           
          //Serial.println(F("day"));          
          elements[BREAK_LIGHT].next = OFF;          
          elements[FRONT_LIGHT].next = OFF;
          break;

        case (ON):           
          //Serial.println(F("day"));          
          elements[BREAK_LIGHT].next = ON;          
          elements[FRONT_LIGHT].next = ON;
          break; 
      }
}

void checkBattery(){
   
  //Serial.println(l);
  
    dvalues[0] = analogRead(BATTERY_PIN);
    byte bperc = (byte)(100*((BAT_LEVEL_READ - dvalues[0])/(float)(BAT_LEVEL_READ - BAT_FULL_LEVEL)));    
    if (bperc < 20){
      //send message
      if (bat_send_timer < (millis() + 5000 )){ //wait 5 sec to send again      
        car.sendLowBattery(serverStation, nodeid);
        bat_send_timer = millis();
      }      
    }     
}

uint8_t setAndCheckParam(uint8_t element,
                         uint8_t num_param,
                         uint8_t p0,
                         uint8_t p1, 
                         uint8_t p2, 
                         uint8_t p3){
  return setAndCheckParam(element, num_param, p0, p1, p2, p3, 0, 0, 0);
}

uint8_t setAndCheckParam(uint8_t element,
                         uint8_t num_param,
                         uint8_t p0,
                         uint8_t p1, 
                         uint8_t p2, 
                         uint8_t p3,
                         uint8_t p4){
  return setAndCheckParam(element, num_param, p0, p1, p2, p3, p4, 0, 0);
}

uint8_t setAndCheckParam(uint8_t element,
                         uint8_t num_param,
                         uint8_t p0,
                         uint8_t p1, 
                         uint8_t p2, 
                         uint8_t p3,
                         uint8_t p4,
                         uint8_t p5){
  return setAndCheckParam(element, num_param, p0, p1, p2, p3, p4, p5, 0);
}

uint8_t setAndCheckParam(uint8_t element,
                        uint8_t num_param,
                        uint8_t p0,
                        uint8_t p1, 
                        uint8_t p2, 
                        uint8_t p3, 
                        uint8_t p4,
                        uint8_t p5,
                        uint8_t p6){
    uint8_t params[MAXPARAMS];
    uint8_t r;
    params[0] = p0;
    params[1] = p1;
    params[2] = p2;
    params[3] = p3;
    params[4] = p4;
    params[5] = p5;
    params[6] = p6;
    r = saveParameterToEprom(params , num_param , element);  
    if (r != 0 ){
    car.sendACKMessage(serverStation, nodeid, element, r);
        return r;
    }
    return 0;
}

void setDefaultParams(){  
  uint8_t r;
  bool ok = true;
  /*
  MOTOR
  params[0] = 30; max speed %
  params[1] = 250; base breaking time ms
  params[2] = 4; acc time(ms)=this*params[1].time to reach the max speed after start
  params[3] = 3; breaking time(ms)=this*params[1]
  params[4] = 15; Value that below this the motor does/should not move anymore. Used by IR
  */
  if (setAndCheckParam(MOTOR, MAXPARAMS, 30, 250, 10, 8, 15) != 0){
      ok = false;
  }
  
  /*
  front light
  params[0] = 50; max bright % 
  params[1] = 20; base blink
  params[2] = 20; blink time=this*base blink
  params[3] = 10; blink time emergency=this*base blink 
  */
  if (setAndCheckParam(FRONT_LIGHT, MAXPARAMS, 10, 20, 20, 20) != 0){
      ok = false;
  }
  
  /*
  break light
  params[0] = 50; max bright %
  params[1] = 30; base blink
  params[2] = 30; blink time=this*base blink
  params[3] = 20; blink time emergency=this*base blink
  */
  if (setAndCheckParam(BREAK_LIGHT, MAXPARAMS, 10, 30, 20, 20) != 0){
      ok = false;
  }  

  /*
  left light
  params[0] = 50; max bright %
  params[1] = 20; base blink
  params[2] = 20; blink time=this*base blink
  params[3] = 10; blink time emergency=this*base blink  
  */
  if (setAndCheckParam(LEFT_LIGHT, MAXPARAMS, 10, 15, 15, 10) != 0){
      ok = false;
  }
  /*
  right light
  params[0] = 50; max bright %
  params[1] = 20; base blink
  params[2] = 20; blink time=this*base blink
  params[3] = 10; blink time emergency=this*base blink  
  */
  if (setAndCheckParam(RIGHT_LIGHT, MAXPARAMS, 10, 15, 15, 10) != 0){
      ok = false;
  }

  /*
  sirene light
  params[0] = 50; max bright %
  params[1] = 20; base blink
  params[2] = 20; blink time=this*base blink
  params[3] = 3; blink time emergency=this*base blink
  */
  if (setAndCheckParam(SIRENE_LIGHT, MAXPARAMS, 20, 20, 20, 10) != 0){
      ok = false;
  }  

  /*
  reed
  params[0] = 0; action when detected: 0=stop,1=blinking
  params[1] = 0; toogle 0 is default
  params[2] = 0; spare
  params[3] = 0; spare 
  */
  if (setAndCheckParam(REED, 0, 0, 0, 0, 0) != 0){
      ok = false;
  }  

  /*
  ir receive
  params[0] = 150; Sensitivity 1 to 255. It will become a flot 85/100. Used to control the result of IR reading
  params[1] = 5; RATE Accelarion and breaking rate
  params[2] = 100; A value used to compare the IR levels with to decide to act or not
  params[3] = STOP_FORCE The IR reading indicating the car should stop
  params[4] = 75 DISTANCE The value subtracted to stop force to indicate the car should start breaking
  params[5] = 15 REFRESH_TIME 10 millis * the value here 
  params[6] = VARIATION interval that no action is taken
  */
  if (setAndCheckParam(IR_RECEIVE, MAXPARAMS, 150, 5, 100, 75, 0, 15, 10) != 0){
      ok = false;
  }

  /*
  ir send
  params[0] = 30; time in ms multiplied by 10 to send a message %
  params[1] = 1; how many times to send a message in sequence %
  */
  if (setAndCheckParam(IR_SEND, 2, 30, 2, 0, 0) != 0){
      ok = false;
  }

  if (ok){
     car.sendACKMessage(serverStation, nodeid, 254, 0);
  }

}

void setInitParams(){
  #ifdef DEBUG_CAR
  Serial.println(F("Loading from eprom"));
  #endif
  byte i = 0;
  for (i = 0; i < NUM_ELEMENTS; i++) {      
      uint8_t r = getParameterFromEprom(elements[i].params, elements[i].total_params, i);
      #ifdef DEBUG_CAR
        Serial.println(F("get epron"));
        if ( r != 0){        
            Serial.println(F("Failed to load eprom for element "));
            Serial.println(i);       
        }              
      #endif
  }
  #ifdef DEBUG_CAR           
  dumpParameters();
  #endif
}

void dumpParameters(){
  #ifdef DEBUG_CAR
    for (byte i=0;i<NUM_ELEMENTS;i++){
      Serial.println(F("params element: "));
      Serial.print(i);
      Serial.println(F("\t"));
      for (byte j=0;j<elements[i].total_params;j++){
        Serial.print(elements[i].params[j]);
        Serial.println(F("\t"));
      }
      Serial.println();
    }
  #endif
}

void setPWM(byte idx,byte init_val,int p1, int p2){  
  #ifdef DEBUG_CAR
    Serial.println(F("pwm"));    
  #endif
  SoftPWMSetFadeTime(elements[idx].port, p1, p2);
  SoftPWMSet(elements[idx].port, init_val);  
}

void initElements() {
  #ifdef DEBUG_CAR
    Serial.println(F("Ini elements"));
  #endif
  elements[MOTOR].total_params = 4;
  elements[FRONT_LIGHT].total_params = 4;
  elements[BREAK_LIGHT].total_params = 4;
  elements[LEFT_LIGHT].total_params = 4;
  elements[RIGHT_LIGHT].total_params = 4;
  elements[SIRENE_LIGHT].total_params = 4;
  elements[REED].total_params = 4;
  elements[IR_RECEIVE].total_params = 5;
  elements[IR_SEND].total_params = 2;
  setInitParams();

  //set initial state
  #ifdef DEBUG_CAR
    Serial.println(F("all OFF"));
  #endif

  byte j;
  for (j=0;j<NUM_ELEMENTS;j++){
    elements[j].state = OFF;
    elements[j].next = OFF;
  }
  byte i = MOTOR;
  int aux, aux1;
  //motor  
  #ifdef DEBUG_CAR
    Serial.println(F("set motor"));
  #endif

  elements[i].obj = MOTOR;
  elements[i].controller = &controlMotor;
  elements[i].port = MOTOR_PIN;  
  aux = elements[i].params[1] * elements[i].params[2];
  aux1 = elements[i].params[1] * elements[i].params[3];
  setPWM(i,0,aux,aux1);  
  
  //front light
  i = FRONT_LIGHT;
  elements[i].obj = FRONT_LIGHT;
  elements[i].port = FRONT_LIGHT_PIN; 
  elements[i].controller = &controlBlinkLed;  
  aux = elements[i].params[1]*elements[i].params[2];
  setPWM(i,0,aux,aux);   

  //break light
  i = BREAK_LIGHT;
  elements[i].obj = BREAK_LIGHT;
  elements[i].port = BREAK_LIGHT_PIN;  
  elements[i].controller = &controlBlinkLed;   
  aux = elements[i].params[1]*elements[i].params[2];
  setPWM(i,0,aux,aux);   


  //left light
  i = LEFT_LIGHT;  
  elements[i].obj = LEFT_LIGHT;
  elements[i].port = LEFT_LIGHT_PIN;
  elements[i].controller = &controlBlinkLed;   
  aux = elements[i].params[1]*elements[i].params[2];
  setPWM(i,0,aux,aux);   

  //right light
  i = RIGHT_LIGHT; 
  elements[i].obj = RIGHT_LIGHT;
  elements[i].port = RIGHT_LIGHT_PIN;
  elements[i].controller = &controlBlinkLed;   
  aux = elements[i].params[1]*elements[i].params[2];
  setPWM(i,0,aux,aux);   

  //sirene light
  i = SIRENE_LIGHT; 
  elements[i].obj = SIRENE_LIGHT;
  elements[i].port = SIRENE_LIGHT_PIN;
  elements[i].controller = &controlBlinkLed;   
  aux = elements[i].params[1]*elements[i].params[2];
  setPWM(i,0,aux,aux);   

  //REED
  i = REED; 
  elements[i].obj = REED;
  elements[i].port = REED_PIN;
  elements[i].controller = &controlReed;    

  //IR_RECEIVE
  i = IR_RECEIVE;  
  elements[i].obj = IR_RECEIVE;
  elements[i].port = IR_RECEIVE_PIN;
  elements[i].controller = &controlIRReceive;      

  //IR_SEND
  i = IR_SEND;  
  elements[i].obj = IR_SEND;
  elements[i].port = IR_SEND_PIN;
  elements[i].controller = &controlIRSend; 
  elements[i].total_params = 2;
  #ifdef DEBUG_CAR
    Serial.println(F("all set"));
  #endif
}

void controlAux(ELEMENTS * element) {
  return;
}

int readIrLevel(byte readtimes, uint8_t sentivity) {
  irSum = 0;
  irAux = sentivity / 100;
  for (byte i = 0; i < readtimes; i++) {
    irSum += analogRead(IR_RECEIVE_PIN);
    delayMicroseconds(IR_READ_DELAY);
  }
  irAux = (irSum / readtimes) * sentivity;
  if (irAux < 0) irAux = 0;
  return map((int)irAux, 0, 1023, 0, 255);    
}

void speedcontrol(int f, ELEMENTS *ir) {
    /*
    ir receive
    params[0] = 150; Sensitivity 1 to 255. It will become a flot 85/100. Used to control the result of IR reading
    params[1] = 5; RATE Accelarion and breaking rate
    params[2] = 100; A value used to compare the IR levels with to decide to act or not
    params[3] = STOP_FORCE The IR reading indicating the car should stop
    params[4] = 75 DISTANCE The value subtracted to stop force to indicate the car should start breaking
    params[5] = REFRESH_TIME 10 millis * the value here
    params[6] = VARIATION interval that no action is taken
    */
    /*
    the concept here is to understand the ir sensor as a counter
    force to the car which forces it to decrease the speed
    */
    byte motor = elements[MOTOR].actual_pwm_val; 
    // if a threshold is crossed, drop the speed to half of it was
    if (f >= ir->params[3]) {
      irAux = 0.50 * motor;
      motor = (int)irAux;
    }
    else {
      // some variation here is allowed, it has the objective to smooth the speed control
      distance = f + ir->params[4];
      if (abs(distance - ir->params[3]) < ir->params[6] ) {
        // do nothing
        motor--;
      }
      else if ( distance < ir->params[3]) {
        // if speed is less than the turnoff value we keep it there
        if (motor < elements[MOTOR].params[4]) motor = elements[MOTOR].params[4];
        motor += ir->params[1];
      }
      else if (distance >= ir->params[3]) {
        if (motor >= elements[MOTOR].params[4]){
          motor -= ir->params[1];
        }
      }      
    }

    // set the motor to the max value allowed if it is too big  
    if (motor > elements[MOTOR].params[0]) motor = elements[MOTOR].params[0];
    // set the motor to zero if the value is less than the threshold  
    if (motor < elements[MOTOR].params[4] ) motor = 0; 
    
    SoftPWMSetPercent(elements[MOTOR].port, motor); 
    elements[MOTOR].actual_pwm_val = motor; 
}

// Control the car distance based on IR sensor
void controlIRReceive(ELEMENTS * element) {

  /*
  ir receive
  params[0] = 150; Sensitivity 1 to 255. It will become a flot 85/100. Used to control the result of IR reading
  params[1] = 5; RATE Accelarion and breaking rate
  params[2] = 100; A value used to compare the IR levels with to decide to act or not
  params[3] = STOP_FORCE The IR reading indicating the car should stop
  params[4] = 75 DISTANCE The value subtracted to stop force to indicate the car should start breaking
  params[5] = REFRESH_TIME 10 millis * the value here
  params[6] = VARIATION interval that no action is taken
  */

  if (actime - element->auxTime < element->params[5] * IR_WAIT_TIME){
    force = readIrLevel(5, element->params[0]);
    speedcontrol(force, element);
    element->auxTime = actime;    
  }

}

void controlIRSend(ELEMENTS * element){
  #if defined(__AVR_ATmega328PB__)
    if (actime - element->auxTime > (element->params[0] * 10)){  
      #ifdef DEBUG_CAR
      //Serial.print(F("SEND IR "));
      //Serial.print(element->params[1]);
      //Serial.println(F(" times"));
      #endif      
      for (byte i = 0; i < element->params[1]; i++){
        irsend.sendSony(nodeid, 16);
        
      }      
      element->auxTime = actime;
    }
  #endif
}

void controlReed(ELEMENTS * element) {
  
  //toggle if the param 1 is set
  if (digitalRead(REED_PIN) == LOW ) {
    if (element->params[1] == 0){
      element->state = OFF; //no magnet  
    }else{
      element->state = ON; //magnet
    }    
  }
  else {    
    if (element->params[1] == 0){
      element->state = ON; //magnet  
    }else{
      element->state = OFF; //no magnet
    } 
  }

  switch (element->params[0]){
    case (0)://control motor
         if (element->state == ON ){
            //stop the car
            if (element->lastState != ON){
              element->lastState = ON;
              element->next = elements[MOTOR].state;//save motor state
              elements[MOTOR].next = OFF;
            }            
         }
         else {
            if (element->lastState == ON){
              //back the motor to the last state
              element->lastState = OFF;              
              elements[MOTOR].next = element->next;              
            }
         }
    break;
    case (1)://blinking maybe need change
         if (element->state == ON ){            
            if (element->lastState != ON){
              element->lastState = ON;              
              controlBoard(ALL_BLINKING);
            }            
         }
         else {
            if (element->lastState == ON){              
              element->lastState = OFF;              
              controlBoard(NORMAL);              
            }
         }
    break;
    
  }
    
  return;
}

/*
void controlBreakLeds(ELEMENTS * element) {
  //Serial.println(F("controlBreakLeds"));
  long t;
  byte aux;
  t = millis();

  if (element->state == ON && element->next == OFF) {
    element->lastState = element->state;
    element->state = OFF;
    elements->actual_pwm_val = 0;
    SoftPWMSetPercent(element->port, 0);
  }

  if (element->state == OFF && element->next == ON) {
    element->lastState = element->state;
    element->state = ON;
    //aux = (byte)255 * (element->params[0] / 100);
    elements->actual_pwm_val = element->params[0];
    SoftPWMSetPercent(element->port, element->params[0]);
  }

  if (element->state != BLINKING && element->next == BLINKING) {
    element->lastState = element->state;
    element->state = BLINKING;
    element->auxTime = t + element->params[1] * element->params[2];
  }

  if (element->state == BLINKING ) {
    //Serial.println(F("blinking"));
    if (t > element->auxTime) {
      element->auxTime = t + element->params[1] * element->params[2];
      if (element->tempState == OFF) {
        element->tempState = ON;
        aux = element->params[0];
      }
      else {
        element->tempState = OFF;
        aux = 0;
      }
      elements->actual_pwm_val = aux;
      SoftPWMSetPercent(element->port, aux);
    }
  }

  if (element->next == ON) {
    element->lastState = element->state;
    element->state = ON;
    elements->actual_pwm_val = element->params[0];
    SoftPWMSetPercent(element->port, element->params[0]);
  }

  if (element->next == ON) {
    element->lastState = element->state;
    element->state = OFF;
    //analogWrite(element->port, 0);
    elements->actual_pwm_val = 0;
    SoftPWMSetPercent(element->port, 0);
  }

}
*/

void controlBlinkLed(ELEMENTS * element) {
  //Serial.println(F("controlBlinkLed"));
  long t;
  byte aux;
  t = millis();
 
  if (element->next == OFF) {
    element->lastState = element->state;
    element->state = OFF;
    element->actual_pwm_val = 0;
    SoftPWMSetPercent(element->port, 0);
    //analogWrite(element->port, 0);        
  }
  
  if (element->state == OFF && element->next == ON) {
    element->lastState = element->state;
    element->state = ON;
    element->actual_pwm_val = element->params[0];
    SoftPWMSetPercent(element->port, element->params[0]); 
  }

  if (element->state != BLINKING && element->next == BLINKING) {
    element->lastState = element->state;
    element->state = BLINKING;
    element->auxTime = t + element->params[1] * element->params[2]; 
  }

  if (element->state == BLINKING ) {    
    if (t > element->auxTime) {
      element->auxTime = t + element->params[1] * element->params[2];
      if (element->tempState == OFF) {
        element->tempState = ON;
        aux = element->params[0];
      }
      else {
        element->tempState = OFF;
        aux = 0;
      }
      element->actual_pwm_val = aux;
      SoftPWMSetPercent(element->port, aux);
    } 
  }

  if (element->next == ON) {
    element->lastState = element->state;
    element->state = ON;
    element->actual_pwm_val = element->params[0];
    SoftPWMSetPercent(element->port, element->params[0]); 
  }  
  
}
//Control the motor
void controlMotor(ELEMENTS * element) {
  long t;
  byte aux;

  //Serial.println(F("controlMotor"));
  t = millis();
  if (element->state == ON && element->next == OFF) {
    element->lastState = element->state;
    element->state = STOPING;
    element->auxTime = t + element->params[1] * element->params[2];
    element->actual_pwm_val = 0;    
    SoftPWMSetFadeTime(element->port, element->params[1]*element->params[2], element->params[1]*element->params[3]);
    SoftPWMSetPercent(element->port, 0);    
    
    //save the break state    
    elements[BREAK_LIGHT].auxState = elements[BREAK_LIGHT].state;
    element->auxState = elements[BREAK_LIGHT].lastState;
    //turn the break light on
    elements[BREAK_LIGHT].next = ON;    
    //return;
  }
  if (element->state == OFF && element->next == ON) {   
    element->lastState = OFF;
    element->state = ACCELERATING;
    element->auxTime = t + element->params[1] * element->params[3];
    element->actual_pwm_val = element->params[0];
    SoftPWMSetFadeTime(element->port, element->params[1]*element->params[2], element->params[1]*element->params[3]);
    SoftPWMSetPercent(element->port, element->params[0]);
    //return;
  }

  if (element->state == STOPING) {    
    if (t > element->auxTime) {
      element->lastState = ON;
      element->state = OFF;
      //turn the break lights off
      elements[BREAK_LIGHT].next = elements[BREAK_LIGHT].auxState;
      elements[BREAK_LIGHT].state = element->auxState;
      element->tempState=OFF;
    }
    //return;
  }

  if (element->state == ACCELERATING) {
    if (t > element->auxTime) {
      element->lastState = OFF;
      element->state = ON;
      //aux = (byte)255 * (element->params[0] / 100);
      element->actual_pwm_val = element->params[0];
      //analogWrite(element->port, element->params[0]);
      SoftPWMSetPercent(element->port, element->params[0]);
    }
  }
}


/*memory organization
 * each part has MAXPARAMS
 * each param is 1byte
 * node id has 2 bytes
 * the first 2 bytes are the nodeid
 * the next 20 bytes are reserved
 * the first part starts at the byte position 23
 * the next part starts at multiples of MAXPARAMS
 */

uint8_t getParameterFromEprom(byte *params, byte numParams, byte obj) {

  if (numParams > MAXPARAMS) {
    return 1;
  }
  byte i = 0;
  byte val;
  int startpos = MEMORY_REF + (MAXPARAMS * obj);

  if ((startpos + MEMORY_REF) > EPROM_SIZE) {
    #ifdef DEBUG_CAR
          Serial.println(F("Failed to get eprom. eprom size"));
    #endif
    return 2;
  }
  //Serial.println(F("getting element:"));
  //Serial.print(obj);
  //Serial.println();
  for (i = 0; i < numParams; i++) {
    val = EEPROM.read(startpos + i);
    params[i] = val;
    //Serial.print(i);
    //Serial.println(F("\t"));
    //Serial.print(val);
    //Serial.println(F(" pos:"));
    //Serial.print(startpos + i);
    //Serial.println(F("\t"));
  }
  //Serial.println();
return 0;
}

//save the general parameters to the prom
uint8_t saveParameterToEprom(byte *params, byte numParams, byte obj) {

  if (numParams > MAXPARAMS) {
    #ifdef DEBUG_CAR
         // Serial.println(F("Failed to save to eprom maxparam"));
     #endif
    return 1;
  }
  int i = 0;
  byte val = 0;
  int startpos = MEMORY_REF + (MAXPARAMS * obj);

  if ((startpos + MEMORY_REF) > EPROM_SIZE) {
    #ifdef DEBUG_CAR
          //Serial.println(F("Failed to save to eprom. eprom size"));
    #endif
    return 2;
  }
  //Serial.println(F("saving element:"));
  //Serial.print(obj);
  //Serial.println();
  for (i = 0; i < numParams; i++) {    
    //Serial.print(i);
    //Serial.println(F("\t"));
    //Serial.print(params[i]);
    //Serial.println(F(" pos:"));
    //Serial.print(startpos + i);
    //Serial.println(F("\t"));
 
    EEPROM.write(startpos + i, params[i]);    
  }
  //Serial.println();
  return 0;
}

//save the node id to the prom
uint8_t saveNodeIdToEprom(uint16_t node) {  
  byte val0 = highByte(node);  
  byte val1 = lowByte(node);  
   
  EEPROM.write(0,val0);
  EEPROM.write(1,val1); 
 
  return 0;
}

//retrieve the node id from the prom

uint16_t getNodeIdFromEprom() {  
  byte val0 = EEPROM.read(0);
  byte val1 = EEPROM.read(1);  
   
  return word(val0,val1); 
}
