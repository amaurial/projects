
#include "Arduino.h"
#include <csrd.h>
#include <RHReliableDatagram.h>
#include <RH_RF69.h>
#include <SPI.h>
#include <EEPROM.h>
#include <SoftPWM.h>

//PINS
#define FRONT_LIGHT_PIN           A1
#define LEFT_LIGHT_PIN            7//5
#define RIGHT_LIGHT_PIN           8//10
#define SIRENE_LIGHT_PIN          5 //AUX1
#define BREAK_LIGHT_PIN           3//13
#define BREAK_AUX_LIGHT_PIN       11
#define MOTOR_PIN                 4//A4
#define MOTOR_ROTATION_PIN        A3
#define BATTERY_PIN               A5
#define IR_REC_PIN                D6 //AUX2
#define IR_SENDER_PIN             D10 //AUX3
#define MAXPARAMS                 5


#define MEMORY_REF                23
#define EPROM_SIZE                512

typedef struct ELEMENTS *ELEMENT; 
//pointer to the functions that will control the parts
//actual state,next state, parameters,type,num parameters
typedef void(*controllers)(ELEMENT);

//structure that holds all important data
typedef struct ELEMENTS{
  states state;
  states next;
  states tempState;
  objects_enum obj; 
  byte params[MAXPARAMS];
  byte total_params;
  controllers controller;
  long auxTime; 
  byte port;  
};



#define NUM_ELEMENTS 5
struct ELEMENTS elements[NUM_ELEMENTS];

//radio buffer
byte radioBuffer[RH_RF69_MAX_MESSAGE_LEN];
RH_RF69 driver;
//RHReliableDatagram manager(driver, 33);

//car message lib
CSRD car;
long actime;

byte i=0;
byte j=0;
uint8_t sbuffer[MESSAGE_SIZE];
uint8_t recbuffer[MESSAGE_SIZE];
long st;
long count;

void controlBreakLeds(ELEMENTS * element);
void controlBlinkLedLeft(ELEMENTS * element);
void controlBlinkLedRight(ELEMENTS * element);
void controlFrontLight(ELEMENTS * element);
void controlMotor(ELEMENTS * element);
void controlSirene(ELEMENTS * element);

void setup(){
  Serial.begin(115200);
  SoftPWMBegin();
  initElements();
  //car.init(&driver,&manager); 
  //if (!car.init(&driver,&manager)){
  if (!car.init(&driver,NULL)){    
    Serial.println("FAILED");
  }
  driver.setModemConfig(RH_RF69::FSK_Rb250Fd250);
  i=0; 
  count=0;
  st=millis();
  setPins();
  //testPins();
}

void loop(){

  actime=millis();

  for (i=0;i<NUM_ELEMENTS;i++){
    elements[i].controller(&elements[i]);
  }

  //delay(1000);
   /*
  if (i>254){
    i=0;
  } 

 
  if ((millis()-st)>5000){
    Serial.print("msg/s:");
    Serial.println(count/5);
    count=0;
    st=millis();
  }
  
    
   setBuffer();
   //Serial.println("Sending to the server");
   car.sendMessage(sbuffer,MESSAGE_SIZE,1); 

  for (j=0;j<10;j++){
    if (car.getMessage(recbuffer)>0){    
      //Serial.print("from server: ");
      //Serial.print(car.getSender());
      //Serial.print(" size: ");
      //Serial.println(car.getLength());
      //printBuffer();
      //Serial.println();
      count++;
      i++;
      break;
    }
    delay(1);
  }    
  */
}

void initElements(){
  int i=0;
  int aux,aux1;
  //motor  
  elements[i].state=ON;
  elements[i].next=OFF;
  elements[i].obj=MOTOR;  
  elements[i].controller=&controlMotor;
  elements[i].port= MOTOR_PIN;
  //load params
  elements[i].params[0]=100; //max speed %
  elements[i].params[1]=250; //base breaking time ms
  elements[i].params[2]=10; //breaking time(ms)=this*params[1]
  elements[i].params[3]=10; //acc time(ms)=this*params[1].time to reach the max speed after start
  elements[i].total_params=4;
  aux=elements[i].params[1]*elements[i].params[2];
  aux1=elements[i].params[1]*elements[i].params[3];
  SoftPWMSet(MOTOR_PIN,0);
  SoftPWMSetFadeTime(MOTOR_PIN,aux,aux1);
  SoftPWMSetPercent(MOTOR_PIN,elements[i].params[0]);   
  
  delay(2*aux);
   
  //front light
  i=1;
  elements[i].state=OFF;
  elements[i].next=ON;
  elements[i].obj=FRONT_LIGHT;  
  elements[i].port=FRONT_LIGHT_PIN;
  elements[i].controller=&controlFrontLight;
  //load params
  elements[i].params[0]=100; //max bright %  
  elements[i].total_params=1;
  aux=100;
  aux1=100;
  SoftPWMSet(FRONT_LIGHT_PIN,0);
  SoftPWMSetFadeTime(FRONT_LIGHT_PIN,aux,aux1);

  //break light
  i=2;
  elements[i].state=OFF;
  elements[i].next=BLINKING;
  elements[i].obj=BREAK_LIGHT;  
  elements[i].port=BREAK_LIGHT_PIN;
  elements[i].controller=&controlBreakLeds;
  //load params
  elements[i].params[0]=100; //max bright %  
  elements[i].params[1]=30; //base blink
  elements[i].params[2]=30; //blink time=this*base blink
  elements[i].params[3]=20; //blink time emergency=this*base blink
  elements[i].total_params=4;
  aux=elements[i].params[1];  
  SoftPWMSet(BREAK_LIGHT_PIN,0);
  SoftPWMSetFadeTime(BREAK_LIGHT_PIN,aux,aux);
  

  //left light
  i=3;
  elements[i].state=OFF;
  elements[i].next=BLINKING;
  elements[i].obj=LEFT_LIGHT;  
  elements[i].port=LEFT_LIGHT_PIN;
  elements[i].controller=&controlBlinkLedLeft;
  //load params
  elements[i].params[0]=100; //max bright %  
  elements[i].params[1]=20; //base blink
  elements[i].params[2]=20; //blink time=this*base blink
  elements[i].params[3]=10; //blink time emergency=this*base blink
  elements[i].total_params=4;  
  aux=elements[i].params[1];  
  SoftPWMSet(LEFT_LIGHT_PIN,0);
  SoftPWMSetFadeTime(LEFT_LIGHT_PIN,aux,aux);

  //right light
  i=4;
  elements[i].state=OFF;
  elements[i].next=BLINKING;
  elements[i].obj=RIGHT_LIGHT;  
  elements[i].port=RIGHT_LIGHT_PIN;
  elements[i].controller=&controlBlinkLedRight;
  //load params  
  elements[i].params[0]=100; //max bright %  
  elements[i].params[1]=20; //base blink
  elements[i].params[2]=20; //blink time=this*base blink
  elements[i].params[3]=10; //blink time emergency=this*base blink
  elements[i].total_params=4;
  aux=elements[i].params[1];  
  SoftPWMSet(RIGHT_LIGHT_PIN,0);
  SoftPWMSetFadeTime(RIGHT_LIGHT_PIN,aux,aux);

 
}

void setBuffer(){
  sbuffer[0]=i;
  sbuffer[1]=40;
  sbuffer[2]=40;
  sbuffer[3]=40;
  sbuffer[4]=40;
  sbuffer[5]=40;
  sbuffer[6]=40;
  sbuffer[7]=i;
}

void printBuffer(){
  int a;
  for (a=0;a<MESSAGE_SIZE;a++){
    Serial.print(recbuffer[a],HEX);
    Serial.print(" ");
  }
}

void setPins(){
  //pinMode(LEFT_LIGHT_PIN,OUTPUT);
  //pinMode(RIGHT_LIGHT_PIN,OUTPUT);
  //pinMode(BREAK_LIGHT_PIN,OUTPUT);
  //pinMode(SIRENE_LIGHT_PIN,OUTPUT);    
  
}

void testPins(){

  digitalWrite(LEFT_LIGHT_PIN,HIGH);
  delay(1500);
  digitalWrite(LEFT_LIGHT_PIN,LOW);
  
  digitalWrite(RIGHT_LIGHT_PIN,HIGH);
  delay(1500);
  digitalWrite(RIGHT_LIGHT_PIN,LOW);
  
  analogWrite(BREAK_LIGHT_PIN,255);
  delay(1500);
  analogWrite(BREAK_LIGHT_PIN,0);
  
  digitalWrite(SIRENE_LIGHT_PIN,HIGH);
  delay(1500);
  digitalWrite(SIRENE_LIGHT_PIN,LOW);
  
  analogWrite(FRONT_LIGHT,255);
  delay(1500);
  analogWrite(FRONT_LIGHT,0);

  analogWrite(MOTOR_PIN,128);
  delay(1500);
  analogWrite(MOTOR_PIN,0);
  
}

void controlBreakLeds(ELEMENTS * element){
  //Serial.println("controlBreakLeds");
  long t;
  byte aux;
  t=millis();
  
  if (element->state == ON && element->next == OFF){
      element->state = OFF;
      //analogWrite(element->port,0);   
      SoftPWMSetPercent(element->port,0);    
  }
  if (element->state == OFF && element->next == ON){
      element->state = ON;      
      aux=(byte)255*(element->params[0]/100);
      //analogWrite(element->port,aux);   
      SoftPWMSetPercent(element->port,element->params[0]);     
  }

  if (element->state != BLINKING && element->next == BLINKING){
      element->state = BLINKING;      
      element->auxTime = t + element->params[1]*element->params[2];      
  }

  if (element->state == BLINKING ){ 
      Serial.println("blinking"); 
      if (t > element->auxTime){
        element->auxTime = t + element->params[1]*element->params[2];
        if (element->tempState == OFF){
          element->tempState = ON;
          //aux=(byte)255*(element->params[0]/100);
          aux=element->params[0];
        }
        else{
          element->tempState = OFF;
          aux=0;
        }
        //analogWrite(element->port,aux);
        SoftPWMSetPercent(element->port,aux); 
      }
  }

  if (element->next == ON){      
      element->state = ON;
      //aux=(byte)255*(element->params[0]/100);
      //analogWrite(element->port,aux);   
      SoftPWMSetPercent(element->port,element->params[0]);      
  }
  if (element->next == ON){      
      element->state = OFF;      
      analogWrite(element->port,0);
      SoftPWMSetPercent(element->port,0);             
  }
  
}
void controlBlinkLedLeft(ELEMENTS * element){
  //Serial.println("controlBlinkLedLeft");
  long t;
  byte aux;
  t=millis();
  
  if (element->state == ON && element->next == OFF){
      element->state = OFF;
      analogWrite(element->port,0);       
  }
  if (element->state == OFF && element->next == ON){
      element->state = ON;      
      //aux=(byte)255*(element->params[0]/100);
     // analogWrite(element->port,aux);   
      SoftPWMSetPercent(element->port,element->params[0]);     
  }

  if (element->state != BLINKING && element->next == BLINKING){
      element->state = BLINKING;      
      element->auxTime = t + element->params[1]*element->params[2];      
  }

  if (element->state == BLINKING ){ 
      Serial.println("blinking"); 
      if (t > element->auxTime){
        element->auxTime = t + element->params[1]*element->params[2];
        if (element->tempState == OFF){
          element->tempState = ON;
          //aux=(byte)255*(element->params[0]/100);
          aux=element->params[0];
        }
        else{
          element->tempState = OFF;
          aux=0;
        }
        //analogWrite(element->port,aux);
        SoftPWMSetPercent(element->port,aux);
      }
  }

  if (element->next == ON){      
      element->state = ON;
      //aux=(byte)255*(element->params[0]/100);
      //analogWrite(element->port,aux); 
      SoftPWMSetPercent(element->port,element->params[0]);        
  }
  if (element->next == ON){      
      element->state = OFF;      
      //analogWrite(element->port,0);  
      SoftPWMSetPercent(element->port,0);           
  }
}

void controlBlinkLedRight(ELEMENTS * element){
  //Serial.println("controlBlinkLedRight");
  long t;
  byte aux;
  t=millis();
  
  if (element->state == ON && element->next == OFF){
      element->state = OFF;
      //analogWrite(element->port,0);
      SoftPWMSetPercent(element->port,0);         
  }
  if (element->state == OFF && element->next == ON){
      element->state = ON;      
      //aux=(byte)255*(element->params[0]/100);
      //analogWrite(element->port,aux);       
      SoftPWMSetPercent(element->port,element->params[0]);  
  }

  if (element->state != BLINKING && element->next == BLINKING){
      element->state = BLINKING;      
      element->auxTime = t + element->params[1]*element->params[2];      
  }

  if (element->state == BLINKING ){ 
      Serial.println("blinking"); 
      if (t > element->auxTime){
        element->auxTime = t + element->params[1]*element->params[2];
        if (element->tempState == OFF){
          element->tempState = ON;
          //aux=(byte)255*(element->params[0]/100);
          aux=element->params[0];
        }
        else{
          element->tempState = OFF;
          aux=0;
        }
        //analogWrite(element->port,aux);
        SoftPWMSetPercent(element->port,aux);
      }
  }

  if (element->next == ON){      
      element->state = ON;
      //aux=(byte)255*(element->params[0]/100);
      //analogWrite(element->port,aux);  
      SoftPWMSetPercent(element->port,element->params[0]);       
  }
  if (element->next == ON){      
      element->state = OFF;      
      //analogWrite(element->port,0);   
      SoftPWMSetPercent(element->port,0);          
  }    
}

void controlFrontLight(ELEMENTS * element){
  //Serial.println("controlFrontLight"); 
}
void controlMotor(ELEMENTS * element){
  float perc;
  long t;
  byte aux;
  float diff;

  //Serial.println("controlMotor");  
  t=millis();
  if (element->state == ON && element->next == OFF){
      element->state = STOPING;
      element->auxTime = t + element->params[1]*element->params[2];
      //aux=(byte)255*(element->params[0]/100);
      //analogWrite(element->port,aux);  
      SoftPWMSetPercent(element->port,0);     
  }
  if (element->state == OFF && element->next==ON){
      element->state = ACCELERATING;
      element->auxTime = t + element->params[1]*element->params[3];
      //analogWrite(element->port,0);  
      SoftPWMSetPercent(element->port,element->params[0]);      
  }
  
  if (element->state == STOPING){    
    if (t > element->auxTime){
      element->state = OFF;
      //SoftPWMSetPercent(element->port,0);  
      //SoftPWMSetPercent(element->port,0);         
    }
    /*
    else{
      diff=element->auxTime-t;
      perc=diff / element->auxTime;
      analogWrite(element->port,255*perc);
    }
    */
  }

  if (element->state == ACCELERATING){    
    if (t > element->auxTime){
      element->state = ON;
      aux=(byte)255*(element->params[0]/100);
      analogWrite(element->port,aux);       
    }
    /*
    else{
      diff = element->auxTime - t;
      perc= diff / element->auxTime;
      aux=(byte)(255*element->params[0]/100)*(1-perc);
      analogWrite(element->port,aux);      
    }
    */
  }
  
}
void controlSirene(ELEMENTS * element){
  //Serial.println("controlSirene");
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

void getParameterFromEprom(byte *params,byte numParams,objects_enum obj){

  if (numParams>MAXPARAMS){
    return;
  }
  int i=0;
  byte val;  
  int startpos=MEMORY_REF+(MAXPARAMS*obj);

  if ((startpos+MEMORY_REF)>EPROM_SIZE){
    return;
  }
  
  for (i=0;i<MAXPARAMS;i++){
    val=EEPROM.read(startpos+i);
    params[i]=val;
  }    
}

void saveParameterToEprom(byte *params,byte numParams,objects_enum obj){

  if (numParams>MAXPARAMS){
    return;
  }
  int i=0;
  byte val;  
  int startpos=MEMORY_REF+(MAXPARAMS*obj);

  if ((startpos+MEMORY_REF)>EPROM_SIZE){
    return;
  }
    
  for (i=0;i<MAXPARAMS;i++){
    EEPROM.write(startpos+i,params[i]);
    params[i]=val;
  }    
}



