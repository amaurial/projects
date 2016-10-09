#include "Arduino.h"
#include <csrd.h>
#include <RHReliableDatagram.h>
#include <RH_RF69.h>
#include <SPI.h>
#include <EEPROM.h>
#include <SoftPWM.h>
#include <SoftwareServo.h>

//uncomment for debug message
//#define DEBUG_CAR 1;

//PINS
#define STEERING_PIN              7
#define FRONT_LIGHT_PIN           6//7 A1
#define LEFT_LIGHT_PIN            5
#define RIGHT_LIGHT_PIN           4
#define SIRENE_LIGHT_PIN          A3 //AUX2
#define BREAK_LIGHT_PIN           3//13
#define REED_PIN                  A2//AUX1
#define MOTOR_PIN                 A4
#define MOTOR_PIN1                A5
#define MOTOR_ROTATION_PIN        A6//A3
#define BATTERY_PIN               A7//A5
#define IR_RECEIVE_PIN            6 //AUX3
#define IR_SEND_PIN               A5 //AUX4
#define CHARGER_PIN               A1

#define BAT_FULL_LEVEL            610 //analog read equivalent to 4V
#define BAT_LEVEL_READ            680 //by tests 688 the motor stops

#define MAXPARAMS                 5

#define MEMORY_REF                23
#define EPROM_SIZE                512

typedef struct ELEMENTS *ELEMENT;

//pointer to the functions that will control the parts
//actual state,next state, parameters,type,num parameters
typedef void(*controllers)(ELEMENT);

//structure that holds all important data
typedef struct ELEMENTS {
  states state;
  states next;
  states tempState;
  states lastState;
  states auxState;
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
 
//radio buffer
byte radioBuffer[RH_RF69_MAX_MESSAGE_LEN];
RH_RF69 driver;
//RHReliableDatagram manager(driver, 33);

//car message lib
CSRD car;

/*
 * aux vars
 */
long actime;
long st;
float motor_rotation;
int msamples;

/*
 * battery vars
 */
long bat_send_timer = 0; //time between lowbat level

//incomming and outcomming buffer
uint8_t sbuffer[MESSAGE_SIZE];
uint8_t recbuffer[MESSAGE_SIZE];

/*
 * node status
 */
STATUS status;

/*
 * node number and group
 */
uint16_t nodeid = 999;
uint8_t group = 1;
uint8_t serverStation = 1;

/*
 * message flag
 */
bool newMessage = false;
/*
 * acquired flag
 */
bool acquired = false;
uint8_t acquire_server = 0;

/*
 * time variables
 */
long refresh_registration;
long last_registration;
byte LAST_MESSAGE_TIMEOUT = 30; //30 seconds
double long last_message = 0;
/*
 * functions to control the elements
 */
void controlBreakLeds(ELEMENTS * element);
void controlBlinkLed(ELEMENTS * element);
void controlMotor(ELEMENTS * element);
void controlAux(ELEMENTS * element);
void controlReed(ELEMENTS * element);

/*
 * steering variable
 * and call back function passed to softPWM
 * to refresh the soft servo lib
 */
SoftwareServo myservo;
byte midang = 92;
byte lastAng = 0;

void refreshSoftServo(int a){
  SoftwareServo::refresh();
}


void setup() {
  boolean r=false;
  #ifdef DEBUG_CAR
  Serial.begin(19200);
  delay(100);
  #endif

  /*
   * charger pin and reed pin 
   */
  pinMode(CHARGER_PIN, INPUT);  
  pinMode(REED_PIN, INPUT_PULLUP);
  
  /*
   * get node number ffrom eprom
   */   
  uint16_t n=getNodeIdFromEprom();
  if (n > 999) nodeid = 999;
  else nodeid = n;
  
  /*
   * Start steering
   */
  myservo.write(midang);
  myservo.attach(STEERING_PIN);
  SoftPWMBegin(SOFTPWM_NORMAL,&refreshSoftServo);

  //if (!car.init(&driver,&manager)){

  /*
   * Start the radio
   * try 10 times if failed
   */
  for (byte a=0;a<10;a++){
    if (car.init(&driver, NULL)) {
        r=true;
        //driver.setTxPower(17);
        break;
    }
    delay(200);
  }
   
  if (r==false) {
    #ifdef DEBUG_CAR
    Serial.println("FAILED THE RADIO");
    #endif
    //turn on the sirene to blink indicating failure
    elements[SIRENE_LIGHT].next = BLINKING;
  }  
  driver.setModemConfig(RH_RF69::FSK_Rb250Fd250);
  
  status = NOT_REGISTERED;
    
  initElements();
  randomSeed(analogRead(A0));
  refresh_registration = random(200, 5000);
  
  #ifdef DEBUG_CAR
  Serial.println("START CLIENT");
  #endif
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
    if (newMessage) {
      last_message = actime;
      #ifdef DEBUG_CAR
        dumpMessage();
      #endif
      
      confirmRegistrationMessage();      
      checkBroadcastRegister();       
    }  
      
    sendRegistrationMessage();
    
    if (newMessage){            
      checkMsgRestoreDefault();             
      checkMsgWriteParameter();      
      checkAction();     
      checkQuery();
      
      if (status == ACTIVE){
        setNextOperation();
      }
    }
    dvalues[1] = analogRead(MOTOR_ROTATION_PIN);    
    checkBattery();  

    if ((actime - last_message) >= LAST_MESSAGE_TIMEOUT * 1000 ){
      acquired = false;
    }
    if (!acquired) {      
      //myservo.detach();
      //car.sendAddressedActionMessage(serverId, car.getNodeNumber() , BOARD, AC_RELEASE, 0, 0);
    }    
}

/*
/* Print the received message
*/
#ifdef DEBUG_CAR
void dumpMessage() {	  
	  Serial.println("New Message");
	  car.getMessageBuffer(recbuffer);
	  for (byte i = 0; i < MESSAGE_SIZE; i++) {
	    Serial.print (recbuffer[i]);
	    Serial.print ("   ");
	  }
	  Serial.println();	
}
#endif

/* restore default parameters */
void checkMsgRestoreDefault(){  
    if (car.isRestoreDefaultConfig(nodeid)){
       #ifdef DEBUG_CAR
            Serial.println("Restore default values");
      #endif
       setDefaultParams();
       initElements();
    }
}

/* confirm we received a registration */
void confirmRegistrationMessage(){
  if (status == WAITING_REGISTRATION) {      

    if (car.isStatus() && car.getNodeNumber() == serverStation && car.getStatus() == ACTIVE) {
      status = ACTIVE;
      #ifdef DEBUG_CAR
            Serial.println("STATUS ACTIVE");
      #endif
    }
  }
}

/* check broadcast register */
void checkBroadcastRegister(){
  if (car.isBroadcastRegister() && car.isMyGroup(group) ){
     /* set the timer */
     #ifdef DEBUG_CAR
            Serial.println("BREG received status WAITING REG");
      #endif
     status = WAITING_REGISTRATION;
      /* set timer */
      randomSeed(analogRead(A0));
      refresh_registration = random(100, 3000);
      last_registration = millis();
  }
}

/* send initial registration message */
void sendRegistrationMessage(){
   
   if ( status == NOT_REGISTERED ||
       ( status == WAITING_REGISTRATION && (actime - last_registration) > refresh_registration) ) {          
          
      /* send message. change the status */
      #ifdef DEBUG_CAR
            Serial.println("send REG request");
      #endif
      car.sendInitialRegisterMessage(serverStation, nodeid, ACTIVE, 0, 0, 0);
      status = WAITING_REGISTRATION;
      /* set timer */
      randomSeed(analogRead(A0));
      refresh_registration = random(200, 5000);
      last_registration = millis();     
  }
}

/* deal with Action messages */
void checkAction(){    
    if (car.isAction()){       
       if ( (car.isAddressed() && (car.getNodeNumber() == nodeid)) || (car.isBroadcast() && car.isMyGroup(group)) ){
          //if (car.getElement() != BOARD){
	          uint8_t action=car.getAction();
             uint8_t e, p, v, v1;
             int aux, aux1;
             
             if (action == AC_SET_PARAM){
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
                    //TODO
                 }
          }
          else if (action == AC_SET_PARAM){
              e = car.getElement();
              p = car.getParamIdx();
              v = car.getVal0();                 
              if (e == MOTOR && e == 0) {   
                  SoftPWMSetFadeTime(elements[e].port, 0, 0);
                  //SoftPWMSet(elements[idx].port, init_val); 
                  //setPWM(e,elements[e].actual_pwm_val,0,0);                   
                  SoftPWMSetPercent(elements[e].port,v);
                  return;
              }
          }          
          else if (action == AC_ACQUIRE){
              if (acquired){
                 //uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t action,uint8_t val0,uint8_t val1);
                 car.sendAddressedActionMessage(serverStation, nodeid,
                                            BOARD, AC_FAIL, 
                                            highByte(acquire_server), lowByte(acquire_server));
              }
              else{
                acquire_server = word(car.getVal0(), car.getVal1());
                acquired = true;
                car.sendAddressedActionMessage(serverStation, nodeid, BOARD, AC_ACK, 0, 0);
                myservo.write(midang);
                myservo.attach(STEERING_PIN);
              }
          } 
          else if (action == AC_RELEASE){
              if (acquired){
                acquired = false;
                car.sendAddressedActionMessage(serverStation, nodeid, BOARD, AC_ACK, 0, 0);
              }
          }
          else if (action == AC_MOVE){              
              v = car.getVal0();  
              v1 = car.getVal1();                               
              if (e == MOTOR || e == 0) {
                  //byte d = car.getVal1();
                  if (v1 == 0){
                    elements[MOTOR].port = A4;
                  }
                  else{
                    elements[MOTOR].port = A5;
                  }
                  if (v1 == elements[MOTOR].params[4]){                    
                    SoftPWMSetPercent(elements[MOTOR].port,v);                     
                  }
                  else {
                    if (v1 == 0){
                      SoftPWMSetPercent(A5,0);                      
                    }
                    else{
                      SoftPWMSetPercent(A4,0);                      
                    }
                    elements[MOTOR].params[4] = v1;
                    SoftPWMSetPercent(elements[MOTOR].port,v);                    
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

/* deal with query messages */
void checkQuery(){  
    if (car.isStatus()){
      #ifdef DEBUG_CAR
        Serial.println("st msg");
      #endif
       if ( car.getNodeNumber() == nodeid ){
	        uint8_t sttype=car.getStatusType();          
          if (sttype == STT_QUERY_VALUE){
              #ifdef DEBUG_CAR
                Serial.println("qry msg");
              #endif
              uint8_t e = car.getElement();
              uint8_t p0 = car.getVal0();
              uint8_t p1 = car.getVal1();
              uint8_t p2 = car.getVal2();
              
              switch (e){
                case BOARD:
                    byte charger;
                    charger = 0;
                    //board has only 3 params: battery level, motor rotation, charger level                  
                    if ((p0 < 3) && (p1 < 3) && (p2 < 3)){
                      if (p2 == 2){                          
                          if (digitalRead(CHARGER_PIN) == HIGH ){
                            charger = 1;
                          }
                      }
                      /*
                       * 0 -> battery 1->motor
                       * 4v -> 610 charged 3.3v -> 640 ; 0v ->1023
                       * 688 - motor stopped
                      */
                      byte bperc = (byte)(100*((BAT_LEVEL_READ - dvalues[p0])/(float)(BAT_LEVEL_READ-BAT_FULL_LEVEL)));
                      #ifdef DEBUG_CAR
                         Serial.println("Send battery msg");
                      #endif
                      byte v= dvalues[p1]*0.049;    
                     
                      #ifdef DEBUG_CAR                  
                         Serial.println(bperc);Serial.println(v);Serial.println(charger);                                       
                      #endif
                      
                      car.sendAddressedStatusMessage(STT_ANSWER_VALUE, serverStation, nodeid, e, bperc, v, charger);                        
                    }
                    else{
                      car.sendAddressedStatusMessage(STT_QUERY_VALUE_FAIL, serverStation, nodeid, e, p0, p1, p2);  
                    }                    
                break;
                default:
                {
                  if (e >= NUM_ELEMENTS ){
                    car.sendAddressedStatusMessage(STT_QUERY_VALUE_FAIL, serverStation, nodeid, e, 0, 0, 0);  
   		            }
          		    //sanity check on params
          		    uint8_t val0,val1,val2;
          		    if (p0 < elements[e].total_params){
          			    val0=elements[e].params[p0];
          		    }			
          		    else val0=RP_FILLER;
          		    
          		    if (p1 < elements[e].total_params){
          			    val1=elements[e].params[p1];
          		    }			
          		    else val1=RP_FILLER;
            
          		    if (p2 < elements[e].total_params){
          			    val2=elements[e].params[p2];
          		    }			
          		    else val2=RP_FILLER;                  
        		      car.sendAddressedStatusMessage(STT_ANSWER_VALUE, serverStation, nodeid, e, val0, val1, val2);  
  		          }//default
              }//switch              
	          }//STT_QUERY_VALUE
        }   
    }             
}

/* deal with write message */
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
                   //send ack
                   car.sendACKMessage(serverStation, nodeid,e,result);
                   if (e!=REED){//Reed has no fade
                   
                       SoftPWMSetFadeTime(elements[e].port, aux, aux1);
                       
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

/* set next operation */
void setNextOperation(){

    if (!(car.isOperation())) {
      return;  
    }  
  
    if (!(car.getNodeNumber() == nodeid || (car.isBroadcast() && car.isMyGroup(group) ))) {
       return;
    }
    #ifdef DEBUG_CAR
    dumpParameters();
    #endif
    
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
          //Serial.println("night");            
          elements[BREAK_LIGHT].next = ON;          
          elements[FRONT_LIGHT].next = ON;
          break; 
        case (DAY):           
          //Serial.println("day");          
          elements[BREAK_LIGHT].next = OFF;          
          elements[FRONT_LIGHT].next = OFF;
          break; 
      }
}

void checkBattery(){  
    dvalues[0] = analogRead(BATTERY_PIN);
    byte bperc = (byte)(100*((BAT_LEVEL_READ - dvalues[0])/(float)(BAT_LEVEL_READ - BAT_FULL_LEVEL)));    
    if (bperc < 20){
      //send message
      if (bat_send_timer < (millis() + 5000 )){ //wait 5 sec to send again
        car.sendLowBattery(serverStation,nodeid);
        bat_send_timer = millis();
      }      
    }     
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
  if (setAndCheckParam(MOTOR,4,30,250,10,8) != 0){
      ok = false;
  }
  
  if (setAndCheckParam(FRONT_LIGHT,4,10,20,20,20) != 0){
      ok = false;
  }
  
  if (setAndCheckParam(BREAK_LIGHT,4,10,30,20,20) != 0){
      ok = false;
  }
  
  if (setAndCheckParam(LEFT_LIGHT,4,10,15,15,10) != 0){
      ok = false;
  }
  
  if (setAndCheckParam(RIGHT_LIGHT,4,10,15,15,10) != 0){
      ok = false;
  }
  
  if (setAndCheckParam(SIRENE_LIGHT,4,20,20,20,10) != 0){
      ok = false;
  }
  
  if (setAndCheckParam(REED,0,0,0,0,0) != 0){
      ok = false;
  }
  
  if (setAndCheckParam(IR_RECEIVE,1,20,0,0,0) != 0){
      ok = false;
  }
  
  if (setAndCheckParam(IR_SEND,1,20,0,0,0) != 0){
      ok = false;
  }

  if (ok){
     car.sendACKMessage(serverStation, nodeid,254,0);
  }

}

void setInitParams(){
  #ifdef DEBUG_CAR
  Serial.println("Loading from eprom");
  #endif
  
  for (byte i = 0; i < NUM_ELEMENTS; i++) {
	  #ifdef DEBUG_CAR 
      Serial.println("get epron");
	  #endif
      if ((getParameterFromEprom(elements[i].params,elements[i].total_params,i)) != 0){
	     #ifdef DEBUG_CAR 
        Serial.print("Failed to load eprom for element ");
        Serial.println(i);
	    #endif
      }
      #ifdef DEBUG_CAR        
        dumpParameters();
      #endif
  }
}

#ifdef DEBUG_CAR

void dumpParameters(){
  for (byte i=0;i<NUM_ELEMENTS;i++){
    Serial.print("params element: ");
    Serial.print(i);
    Serial.print("\t");
    for (byte j=0;j<elements[i].total_params;j++){
      Serial.print(elements[i].params[j]);
      Serial.print("\t");
    }
    Serial.println();
  }
}
#endif

void setPWM(byte idx,byte init_val,int p1, int p2){
  SoftPWMSetFadeTime(elements[idx].port, p1, p2);
  SoftPWMSet(elements[idx].port, init_val);  
}

void initElements() {
  #ifdef DEBUG_CAR
  Serial.println("Ini elements");
  #endif
  elements[MOTOR].total_params = 4;
  elements[FRONT_LIGHT].total_params = 4;
  elements[BREAK_LIGHT].total_params = 4;
  elements[LEFT_LIGHT].total_params = 4;
  elements[RIGHT_LIGHT].total_params = 4;
  elements[SIRENE_LIGHT].total_params = 4;
  elements[REED].total_params = 4;
  elements[IR_RECEIVE].total_params = 1;
  elements[IR_SEND].total_params = 1;
  setInitParams();

  //set initial state
  #ifdef DEBUG_CAR
  Serial.println("all OFF");
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
  Serial.println("set motor");
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
  elements[i].controller = &controlAux;    

  //IR_SEND
  i = IR_SEND;  
  elements[i].obj = IR_SEND;
  elements[i].port = IR_SEND_PIN;
  elements[i].controller = &controlAux; 
  //elements[i].total_params = 1;

  //Board params
  boardParams[0] = 0 ;//degrees for extra steering left
  boardParams[1] = 0 ;//degrees for extra steering right
  boardParams[2] = 0 ;
  boardParams[3] = 0 ;
  boardParams[4] = 0 ;
  
  #ifdef DEBUG_CAR
  Serial.println("all set");
  #endif
}

void controlAux(ELEMENTS * element) {
  return;
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
    case (1)://blinking
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

void controlBreakLeds(ELEMENTS * element) {
  //Serial.println("controlBreakLeds");
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
void controlBlinkLed(ELEMENTS * element) {
  
  long t;
  byte aux;
  t = millis();
 
  if (element->next == OFF) {
    element->lastState = element->state;
    element->state = OFF;
    element->actual_pwm_val = 0;
    SoftPWMSetPercent(element->port, 0);           
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
/* Control the motor */
void controlMotor(ELEMENTS * element) {
  long t;
  byte aux;
  
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
          Serial.println("Failed to get eprom. eprom size");
    #endif
    return 2;
  }
  
  for (i = 0; i < numParams; i++) {
    val = EEPROM.read(startpos + i);
    params[i] = val;    
  }
  
return 0;
}

/* save the general parameters to the prom */
uint8_t saveParameterToEprom(byte *params, byte numParams, byte obj) {

  if (numParams > MAXPARAMS) {
    #ifdef DEBUG_CAR
         // Serial.println("Failed to save to eprom maxparam");
     #endif
    return 1;
  }
  int i = 0;
  byte val = 0;
  int startpos = MEMORY_REF + (MAXPARAMS * obj);

  if ((startpos + MEMORY_REF) > EPROM_SIZE) {
    #ifdef DEBUG_CAR
          //Serial.println("Failed to save to eprom. eprom size");
    #endif
    return 2;
  }
  
  for (i = 0; i < numParams; i++) {  
    EEPROM.write(startpos + i, params[i]);    
  }  
  return 0;
}

/* save the node id to the prom */
uint8_t saveNodeIdToEprom(uint16_t node) {  
  byte val0 = highByte(node);  
  byte val1 = lowByte(node);  
   
  EEPROM.write(0,val0);
  EEPROM.write(1,val1); 
 
  return 0;
}

/* retrieve the node id from the prom */

uint16_t getNodeIdFromEprom() {  
  byte val0 = EEPROM.read(0);
  byte val1 = EEPROM.read(1);  
   
  return word(val0,val1); 
}

