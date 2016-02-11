
#include "Arduino.h"
#include <csrd.h>
#include <RHReliableDatagram.h>
#include <RH_RF69.h>
#include <SPI.h>
#include <EEPROM.h>
#include <SoftPWM.h>

//uncomment for debug message
//#define DEBUG_CAR 1;

//PINS
#define FRONT_LIGHT_PIN           7//A1
#define LEFT_LIGHT_PIN            5
#define RIGHT_LIGHT_PIN           4
#define SIRENE_LIGHT_PIN          A2 //AUX1
#define BREAK_LIGHT_PIN           3//13
#define REAR_BREAK_LIGHT_PIN      A3//AUX2
#define MOTOR_PIN                 A4
#define MOTOR_ROTATION_PIN        A6//A3
#define BATTERY_PIN               A7//A5
#define IR_RECEIVE_PIN            6 //AUX3
#define IR_SEND_PIN               A5 //AUX4

#define MAXPARAMS                 5

#define MEMORY_REF                23
#define EPROM_SIZE                512

#define BAT_LEVEL_READ            200

typedef struct ELEMENTS *ELEMENT;

//pointer to the functions that will control the parts
//actual state,next state, parameters,type,num parameters
typedef void(*controllers)(ELEMENT);

//structure that holds all important data
typedef struct ELEMENTS {
  states state;
  states next;
  states tempState;
  objects_enum obj;
  byte params[MAXPARAMS];
  byte total_params;
  controllers controller;
  long auxTime;
  byte port;
  byte actual_pwm_val;
};
//number of elements
#define NUM_ELEMENTS 9
struct ELEMENTS elements[NUM_ELEMENTS];

//dynamic values

#define D_VALUES 5
uint16_t dvalues[D_VALUES];

//radio buffer
byte radioBuffer[RH_RF69_MAX_MESSAGE_LEN];
RH_RF69 driver;
//RHReliableDatagram manager(driver, 33);

//car message lib
CSRD car;

//aux vars
long actime;
long st;
long count;
float motor_rotation;
int msamples;

//battery vars
int bat_reads = 0;
int bat_level = 0;

//incomming and outcomming buffer
uint8_t sbuffer[MESSAGE_SIZE];
uint8_t recbuffer[MESSAGE_SIZE];

//node status
STATUS status;

//node number and group
uint16_t nodeid = 999;
uint8_t group = 1;
uint8_t serverStation = 1;

//message flag
bool newMessage = false;

//time variables
long refresh_registration;
long last_registration;

//functions to control the elements
void controlBreakLeds(ELEMENTS * element);
void controlBlinkLed(ELEMENTS * element);
void controlFrontLight(ELEMENTS * element);
void controlMotor(ELEMENTS * element);
void controlAux(ELEMENTS * element);

/*
Print nice status messages
*/
void printStatus() {
  switch (status) {
    case WAITING_REGISTRATION:
      Serial.println("status WAITING_REGISTRATION");
      break;
    case ACTIVE:
      Serial.println("status ACTIVE");
      break;
    case NOT_REGISTERED:
      Serial.println("status NOT_REGISTERED");
      break;
    case INACTIVE:
      Serial.println("status INACTIVE");
      break;
    case CHARGING:
      Serial.println("status CHARGING");
      break;
    case PANNE:
      Serial.println("status PANNE");
      break;
    case REGISTERED:
      Serial.println("status REGISTERED");
      break;
  }

}

void setup() {
  boolean r=false;
  Serial.begin(19200);
  delay(100);


  uint16_t n=getNodeIdFromEprom();
  if (n>999){
    nodeid=999;
  }
  else{
    nodeid=n;
  }
  SoftPWMBegin();
  initElements();
  //Serial.println("SETUP");

  //if (!car.init(&driver,&manager)){
  for (byte a=0;a<10;a++){
    if (car.init(&driver, NULL)) {
        r=true;
        break;
    }
      delay(200);
   }
   
   if (r==false) {
    Serial.println("FAILED");
    //turn on the sirene to blink indicating failure
    elements[SIRENE_LIGHT].next = BLINKING;
  }
  driver.setModemConfig(RH_RF69::FSK_Rb250Fd250);

  count = 0;
  //setPins();
  //testPins();

  status = NOT_REGISTERED;

  randomSeed(analogRead(0));
  refresh_registration = random(1, 3000);

  Serial.println("START CLIENT");
}

void loop() {

    actime = millis();

    //carry the active actions
    if (status == ACTIVE) {
      for (byte i = 0; i < NUM_ELEMENTS; i++) {
        elements[i].controller(&elements[i]);
      }
    } 

    newMessage = car.readMessage();
  
    if (newMessage) {
      //#ifdef DEBUG_CAR
        dumpMessage();
      //#endif
      confirmRegistrationMessage();
    }
 
    //send registration message
    sendRegistrationMessage();
    
    if (newMessage){
      checkMsgRestoreDefault();        
      checkMsgWriteParameter();
      checkAction();
      
      if (status == ACTIVE){
        setNextOperation();
      }
    }  

    //if (elements[MOTOR].state==ON){
      msamples++;
      motor_rotation = (motor_rotation + analogRead(MOTOR_ROTATION_PIN))/msamples;
      dvalues[0] = motor_rotation;
      if (msamples > 1000){
        //Serial.print("motor current: ");
        //Serial.println(motor_rotation);
        msamples=0;
        motor_rotation=analogRead(MOTOR_ROTATION_PIN);
        if (motor_rotation==0){
          //TODO
         //send IR stop signal
        }
        else{
          //send the IR moving signal
        }
      }
      dvalues[1] = analogRead(BATTERY_PIN);
      
   // }
}

void dumpMessage() {

#ifdef DEBUG_CAR  
  Serial.println("New Message");
  car.getMessageBuffer(recbuffer);
  for (byte i = 0; i < MESSAGE_SIZE; i++) {
    Serial.print (recbuffer[i]);
    Serial.print ("   ");
  }
  Serial.println();
#endif
}

//restore default parameters
void checkMsgRestoreDefault(){  
    if (car.isRestoreDefaultConfig(nodeid)){
       //#ifdef DEBUG_CAR
            Serial.println("Restore default values");
      //#endif
       setDefaultParams();
       initElements();
    }
}

//confirm we received a registration
void confirmRegistrationMessage(){
  if (status == WAITING_REGISTRATION) {
      #ifdef DEBUG_CAR
          Serial.print("registration message?: \t");
          Serial.print(car.isStatus());
          Serial.print("\t");
          Serial.println(car.getStatus());
          Serial.print("\t");
          Serial.print(car.getNodeNumber());
      #endif

    if (car.isStatus() && car.getNodeNumber() == serverStation && car.getStatus() == ACTIVE) {
      status = ACTIVE;
      #ifdef DEBUG_CAR
            Serial.println("STATUS ACTIVE");
      #endif
    }
  }

}

//send initial registration message
void sendRegistrationMessage(){
   
   if ( status == NOT_REGISTERED ||
       ( status == WAITING_REGISTRATION && (actime - last_registration) > refresh_registration) ||
       (newMessage && car.isBroadcastRegister() && car.isMyGroup(group) )) {
          
          #ifdef DEBUG_CAR
             Serial.println();
             Serial.println("Need to register?");
             printStatus();
             Serial.print("new message: ");
             Serial.println(newMessage);
             Serial.print("broadcast reg: ");
             Serial.println(car.isBroadcastRegister());
             Serial.print("is my group: ");
             Serial.println(car.isMyGroup(group));
             Serial.print("refresh registration time: ");
             Serial.print(actime - last_registration);
             Serial.print("\t");
             Serial.println(refresh_registration);   
             Serial.println();
              Serial.println("Sending initial registration data");
          #endif
      //send message. change the status
      car.sendInitialRegisterMessage(serverStation, nodeid, ACTIVE, 0, 0, 0);
      status = WAITING_REGISTRATION;
      //set timer
      refresh_registration = random(200, 3000);
      last_registration = millis();
     
      #ifdef DEBUG_CAR
          Serial.println("STATUS WAITING REGISTRATION");
      #endif
  }
}

//save parameter
void checkAction(){  
    if (car.isAction()){
       if ( (car.isAddressed() && (car.getNodeNumber() == nodeid)) || (car.isBroadcast() && car.isMyGroup(group)) ){
          //if (car.getElement() != BOARD){
	            uint8_t action=car.getAction();
             
             if (action == AC_SET_PARAM){
                uint8_t e = car.getElement();
             	  uint8_t p = car.getParamIdx();
             	  uint8_t v = car.getVal0();
          		  int aux, aux1;	                  
          
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
                  //TODO
               }
	         }             
                    
       }
    }
}

//save parameter
void checkQuery(){  
    if (car.isStatus()){
       if ( car.getNodeNumber() == nodeid ){
	        uint8_t sttype=car.getStatusType();
          if (sttype == STT_QUERY_VALUE){
              uint8_t e = car.getElement();
              uint8_t p0 = car.getVal0();
              uint8_t p1 = car.getVal0();
              uint8_t p2 = car.getVal0();

              switch (e){
                case BOARD:
                    //board has only 2 params
                    //bool sendAddressedStatusMessage(uint8_t status_code, uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t p0,uint8_t p1,uint8_t p2);
                    if (p0 < 2 && p1 < 2){
                      car.sendAddressedStatusMessage(STT_ANSWER_VALUE,car.getSender(),nodeid, e, dvalues[p0],dvalues[p1], 255);  
                    }
                    else{
                      car.sendAddressedStatusMessage(STT_QUERY_VALUE_FAIL,car.getSender(),nodeid, e, p0,p1,p2);  
                    }                    
                break;
              }
              
	         }
	     }   
    }             
}

//save parameter
void checkMsgWriteParameter(){  
    if (car.isWrite()){
       if ( (car.isAddressed() ) || (car.isBroadcast() && car.isMyGroup(group)) ){
          uint8_t e = car.getElement();
          if ((e != BOARD) && (car.getNodeNumber() == nodeid)){
             
             uint8_t p = car.getParamIdx();
             uint8_t v = car.getVal0();
             int aux, aux1;
             
             if (e < NUM_ELEMENTS){
                if (p < elements[e].total_params){
                   elements[e].params[p]=v;
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
                   SoftPWMSetFadeTime(elements[e].port, aux, aux1);   
                   //setPWM(e,elements[e].actual_pwm_val,aux,aux1);  
                   //send ack
                   car.sendACKMessage(serverStation, nodeid,e,result);
                   if ((e == MOTOR) && (elements[e].state == ON)) {    
                      SoftPWMSetPercent(elements[e].port,elements[e].actual_pwm_val);
                      return;
                  } 
                  else 
                  {
                      SoftPWMSet(elements[e].port, elements[e].actual_pwm_val);          
                  }
                }
             }             
          }
          else if (e == BOARD){
            uint8_t p = car.getParamIdx();
              if (p==0){//Node id
                uint16_t n = word(car.getVal1(),car.getVal0());
                saveNodeIdToEprom(n);
                nodeid=n;
                status = NOT_REGISTERED;
                sendRegistrationMessage();
              }           
          }
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
    
    int e = car.getElement();
    states s = car.convertFromInt(car.getState());
    if (e != BOARD) {
      if (e < NUM_ELEMENTS) {
        elements[e].next = s;            
      }
    }
    else {
      switch (s) {
        case (EMERGENCY):
          //save actual state
          for (byte i = 0; i < NUM_ELEMENTS; i++) {
            elements[i].tempState = elements[i].state;
          }
          elements[(uint8_t)LEFT_LIGHT].next = BLINKING;
          elements[(uint8_t)RIGHT_LIGHT].next = BLINKING;
          elements[(uint8_t)SIRENE_LIGHT].next = BLINKING;
          elements[(uint8_t)BREAK_LIGHT].next = BLINKING;
          elements[(uint8_t)REAR_BREAK_LIGHT].next = BLINKING;
          elements[(uint8_t)FRONT_LIGHT].next = BLINKING;
          elements[(uint8_t)MOTOR].next = OFF;

          break;
        case (NORMAL):
          //get last state
          //save actual state
          for (byte i = 0; i < NUM_ELEMENTS; i++) {
            elements[i].next = elements[i].tempState;
          }
          if (elements[MOTOR].next == STOPING) {
            elements[MOTOR].next = OFF;
          }
          if (elements[MOTOR].next == ACCELERATING) {
            elements[MOTOR].next = ON;
          }

          break;
      }
    }      
}

void checkBattery(){
  int l = analogRead(BATTERY_PIN);
  bat_reads++;  
  bat_level = (bat_level + l) / bat_reads;

  if (bat_reads > BAT_LEVEL_READ){
    if (bat_level >= 643){
      //send message
      car.sendLowBattery(serverStation,nodeid);
    }
    bat_reads = 0;
    bat_level = 0;
  }
}

int getSpeed(){
  int l = analogRead(MOTOR_ROTATION_PIN);
  
}

 uint8_t setAndCheckParam(uint8_t element,uint8_t num_param,uint8_t p0,uint8_t p1, uint8_t p2, uint8_t p3){
    uint8_t params[MAXPARAMS];
    uint8_t r;
    params[0] = p0;//max speed %
    params[1] = p1;//base breaking time ms
    params[2] = p2;//acc time(ms)=this*params[1].time to reach the max speed after start
    params[3] = p3;//breaking time(ms)=this*params[1]
    r = saveParameterToEprom(params , num_param , element);  
    if (r != 0 ){
	car.sendACKMessage(serverStation, nodeid,element,r);
        return r;
    }
    return 0;
}

void setDefaultParams(){  
  uint8_t r;
  bool ok = true;
  //MOTOR
  //params[0] = 50;//max speed %
  //params[1] = 250;//base breaking time ms
  //params[2] = 4;//acc time(ms)=this*params[1].time to reach the max speed after start
  //params[3] = 3;//breaking time(ms)=this*params[1]
  if (setAndCheckParam(MOTOR,4,30,250,10,8) != 0){
      ok = false;
  }
  

  //front light
  //params[0] = 50; //max bright % 
  //params[1] = 20; //base blink
  //params[2] = 20; //blink time=this*base blink
  //params[3] = 10; //blink time emergency=this*base blink 
  if (setAndCheckParam(FRONT_LIGHT,1,20,0,0,0) != 0){
      ok = false;
  }
  
  //break light
  //params[0] = 50; //max bright %
  //params[1] = 30; //base blink
  //params[2] = 30; //blink time=this*base blink
  //params[3] = 20; //blink time emergency=this*base blink
  if (setAndCheckParam(BREAK_LIGHT,4,20,30,30,20) != 0){
      ok = false;
  }  

  //left light
  //params[0] = 50; //max bright %
  //params[1] = 20; //base blink
  //params[2] = 20; //blink time=this*base blink
  //params[3] = 10; //blink time emergency=this*base blink  
  if (setAndCheckParam(LEFT_LIGHT,4,20,20,20,10) != 0){
      ok = false;
  }
  //right light
  //params[0] = 50; //max bright %
  //params[1] = 20; //base blink
  //params[2] = 20; //blink time=this*base blink
  //params[3] = 10; //blink time emergency=this*base blink  
  if (setAndCheckParam(RIGHT_LIGHT,4,20,20,20,10) != 0){
      ok = false;
  }

  //sirene light
  //params[0] = 50; //max bright %
  //params[1] = 20; //base blink
  //params[2] = 20; //blink time=this*base blink
  //params[3] = 3; //blink time emergency=this*base blink
  if (setAndCheckParam(SIRENE_LIGHT,4,20,20,20,10) != 0){
      ok = false;
  }  

  //rear break light
  //params[0] = 50; //max bright %
  //params[1] = 30; //base blink
  //params[2] = 30; //blink time=this*base blink
  //params[3] = 20; //blink time emergency=this*base blink  
  if (setAndCheckParam(REAR_BREAK_LIGHT,4,20,30,30,20) != 0){
      ok = false;
  }  

  //ir receive
  //params[0] = 50; //max bright %  
  if (setAndCheckParam(IR_RECEIVE,1,20,0,0,0) != 0){
      ok = false;
  }

  //ir send
  //params[0] = 50; //max bright %
  if (setAndCheckParam(IR_SEND,1,20,0,0,0) != 0){
      ok = false;
  }

  if (ok){
     car.sendACKMessage(serverStation, nodeid,254,0);
  }

}

void setInitParams(){
  
  for (byte i = 0; i < NUM_ELEMENTS; i++) {
      if (getParameterFromEprom(elements[i].params,elements[i].total_params,i)) != 0){
        Serial.print("Failed to load eprom for element ");
        Serial.println(i);
      }
      #ifdef DEBUG_CAR        
        Serial.print("params element: ");
        Serial.print(i);
        Serial.print("\t");
        for (byte j=0;j<elements[i].total_params;j++){
          Serial.print(elements[i].params[j]);
          Serial.print("\t");
        }
        Serial.println();
      #endif
  }
}

void setPWM(byte idx,byte init_val,int p1, int p2){      
  SoftPWMSetFadeTime(elements[idx].port, p1, p2);
  SoftPWMSet(elements[idx].port, init_val);  
}

void initElements() {
  byte i = MOTOR;
  int aux, aux1;

  elements[MOTOR].total_params = 4;
  elements[FRONT_LIGHT].total_params = 4;
  elements[BREAK_LIGHT].total_params = 4;
  elements[LEFT_LIGHT].total_params = 4;
  elements[RIGHT_LIGHT].total_params = 4;
  elements[SIRENE_LIGHT].total_params = 4;
  elements[REAR_BREAK_LIGHT].total_params = 4;
  elements[IR_RECEIVE].total_params = 1;
  elements[IR_SEND].total_params = 1;
  setInitParams();

  //set initial state
  byte j;
  for (j=0;j<NUM_ELEMENTS;j++){
    elements[j].state = OFF;
    elements[j].next = OFF;
  }
  
  //motor  
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
  //elements[i].controller = &controlFrontLight;  
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

  //REAR_BREAK_LIGHT
  i = REAR_BREAK_LIGHT; 
  elements[i].obj = REAR_BREAK_LIGHT;
  elements[i].port = REAR_BREAK_LIGHT_PIN;
  elements[i].controller = &controlBlinkLed;    
  aux = elements[i].params[1]*elements[i].params[2];
  setPWM(i,0,aux,aux);   

  //IR_RECEIVE
  i = IR_RECEIVE;  
  elements[i].obj = IR_RECEIVE;
  elements[i].port = IR_RECEIVE_PIN;
  elements[i].controller = &controlAux;    

  //IR_SEND
  i = IR_SEND;  
  elements[i].obj = IR_SEND;
  elements[i].port = IR_SEND_PIN;
  elements[i].controller = &controlAux; 
  //elements[i].total_params = 1;
  
}

void setPins() {
  //pinMode(LEFT_LIGHT_PIN,OUTPUT);
  //pinMode(RIGHT_LIGHT_PIN,OUTPUT);
  //pinMode(BREAK_LIGHT_PIN,OUTPUT);
  //pinMode(SIRENE_LIGHT_PIN,OUTPUT);
  //pinMode(2,INPUT);

}

void testPins() {

  digitalWrite(LEFT_LIGHT_PIN, HIGH);
  delay(1500);
  digitalWrite(LEFT_LIGHT_PIN, LOW);

  digitalWrite(RIGHT_LIGHT_PIN, HIGH);
  delay(1500);
  digitalWrite(RIGHT_LIGHT_PIN, LOW);

  analogWrite(BREAK_LIGHT_PIN, 255);
  delay(1500);
  analogWrite(BREAK_LIGHT_PIN, 0);

  digitalWrite(SIRENE_LIGHT_PIN, HIGH);
  delay(1500);
  digitalWrite(SIRENE_LIGHT_PIN, LOW);

  analogWrite(FRONT_LIGHT, 255);
  delay(1500);
  analogWrite(FRONT_LIGHT, 0);

  analogWrite(MOTOR_PIN, 128);
  delay(1500);
  analogWrite(MOTOR_PIN, 0);

}

void controlAux(ELEMENTS * element) {
  return;
}

void controlBreakLeds(ELEMENTS * element) {
  //Serial.println("controlBreakLeds");
  long t;
  byte aux;
  t = millis();

  if (element->state == ON && element->next == OFF) {
    element->state = OFF;
    elements->actual_pwm_val = 0;
    SoftPWMSetPercent(element->port, 0);
  }
  if (element->state == OFF && element->next == ON) {
    element->state = ON;
    //aux = (byte)255 * (element->params[0] / 100);
    elements->actual_pwm_val = element->params[0];
    SoftPWMSetPercent(element->port, element->params[0]);
  }

  if (element->state != BLINKING && element->next == BLINKING) {
    element->state = BLINKING;
    element->auxTime = t + element->params[1] * element->params[2];
  }

  if (element->state == BLINKING ) {
    //Serial.println("blinking");
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
    element->state = ON;
    elements->actual_pwm_val = element->params[0];
    SoftPWMSetPercent(element->port, element->params[0]);
  }
  if (element->next == ON) {
    element->state = OFF;
    //analogWrite(element->port, 0);
    elements->actual_pwm_val = 0;
    SoftPWMSetPercent(element->port, 0);
  }

}
void controlBlinkLed(ELEMENTS * element) {
  //Serial.println("controlBlinkLed");
  long t;
  byte aux;
  t = millis();
 
  if (element->next == OFF) {
    element->state = OFF;
    element->actual_pwm_val = 0;
    SoftPWMSetPercent(element->port, 0);
    //analogWrite(element->port, 0);        
  }
  
  if (element->state == OFF && element->next == ON) {
    element->state = ON;
    element->actual_pwm_val = element->params[0];
    SoftPWMSetPercent(element->port, element->params[0]); 
  }

  if (element->state != BLINKING && element->next == BLINKING) {
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
    element->state = ON;
    element->actual_pwm_val = element->params[0];
    SoftPWMSetPercent(element->port, element->params[0]); 
  }  
  
}


void controlFrontLight(ELEMENTS * element) {
  //Serial.println("controlFrontLight");

  if (element->state == ON && element->next == OFF) {
    element->state = OFF;
    element->actual_pwm_val = 0;
    SoftPWMSetPercent(element->port, 0);
  }
  if (element->state == OFF && element->next == ON) {
    element->state = ON;
    element->actual_pwm_val = element->params[0];
    SoftPWMSetPercent(element->port, element->params[0]);
  }
}
void controlMotor(ELEMENTS * element) {
  long t;
  byte aux;

  //Serial.println("controlMotor");
  t = millis();
  if (element->state == ON && element->next == OFF) {
    element->state = STOPING;
    element->auxTime = t + element->params[1] * element->params[2];
    element->actual_pwm_val = 0;
    SoftPWMSetPercent(element->port, 0);
    //return;
  }
  if (element->state == OFF && element->next == ON) {
    element->state = ACCELERATING;
    element->auxTime = t + element->params[1] * element->params[3];
    element->actual_pwm_val = element->params[0];
    SoftPWMSetPercent(element->port, element->params[0]);
    //return;
  }

  if (element->state == STOPING) {
    if (t > element->auxTime) {
      element->state = OFF;
    }
    //return;
  }

  if (element->state == ACCELERATING) {
    if (t > element->auxTime) {
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
  int i = 0;
  byte val;
  int startpos = MEMORY_REF + (MAXPARAMS * obj);

  if ((startpos + MEMORY_REF) > EPROM_SIZE) {
    #ifdef DEBUG_CAR
          Serial.println("Failed to get eprom. eprom size");
    #endif
    return 2;
  }
  //Serial.print("getting element:");
  //Serial.print(obj);
  //Serial.println();
  for (i = 0; i < numParams; i++) {
    val = EEPROM.read(startpos + i);
    params[i] = val;
    //Serial.print(i);
    //Serial.print("\t");
    //Serial.print(val);
    //Serial.print(" pos:");
    //Serial.print(startpos + i);
    //Serial.print("\t");
  }
  //Serial.println();
return 0;
}


uint8_t saveParameterToEprom(byte *params, byte numParams, byte obj) {

  if (numParams > MAXPARAMS) {
    #ifdef DEBUG_CAR
          Serial.println("Failed to save to eprom maxparam");
     #endif
    return 1;
  }
  int i = 0;
  byte val = 0;
  int startpos = MEMORY_REF + (MAXPARAMS * obj);

  if ((startpos + MEMORY_REF) > EPROM_SIZE) {
    #ifdef DEBUG_CAR
          Serial.println("Failed to save to eprom. eprom size");
    #endif
    return 2;
  }
  //Serial.print("saving element:");
  //Serial.print(obj);
  //Serial.println();
  for (i = 0; i < numParams; i++) {    
    //Serial.print(i);
    //Serial.print("\t");
    //Serial.print(params[i]);
    //Serial.print(" pos:");
    //Serial.print(startpos + i);
    //Serial.print("\t");
 
    EEPROM.write(startpos + i, params[i]);    
  }
  //Serial.println();
  return 0;
}


uint8_t saveNodeIdToEprom(uint16_t node) {  
  byte val0 = highByte(node);  
  byte val1 = lowByte(node);  
   
  EEPROM.write(0,val0);
  EEPROM.write(1,val1); 
 
  return 0;
}

uint16_t getNodeIdFromEprom() {  
  byte val0 = EEPROM.read(0);
  byte val1 = EEPROM.read(1);  
   
  return word(val0,val1); 
}



