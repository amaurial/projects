/*
 * This sketch controls the distance of two cars by
 * measuring the ir sensor and defining it as a force that decreases
 * the speed or increases if the speed is not in place
 */

#include <Arduino.h>
#include <SoftPWM.h>

#define IRPIN A0
#define MOTORPIN A4

#define STOP_FORCE 150//500 ;0 to 255
#define DISTANCE 20//200 // maior mais longe 0 to 255
#define RATE_DOWN 2
#define RATE_UP 2
#define MAX_LEVEL 60//700 percentage
#define VARIATION_ALLOWED 5//100 ; 0 to 255
#define MOTOR_TURNOFF 10//150 // percentage
float SENSITIVITY = 0.5; // in percentage
int motor = MAX_LEVEL;
int lastlevel = 0;
int force = 0;
int p, d = 0;
uint32_t t=0, val=0;
float r=0;
uint16_t wait = 250;//milli
uint32_t actual_time;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);  
  pinMode(MOTORPIN, OUTPUT);
  SoftPWMBegin();  
  // the max value for SoftPWMSet is 255, so we need to map the motor value
  //p = map(motor, 0, 1023, 0, 255);
  //SoftPWMSet(MOTORPIN, p);
  SoftPWMSetPercent(MOTORPIN, motor);
  //analogWrite(MOTORPIN, p);
  Serial.println("start");
  delay(100);
  t = millis();
}

void loop() {
  // put your main code here, to run repeatedly:
  actual_time = millis();
  if (time_is_over(wait, t)) {
    
    force = readlevel(5);  
    Serial.print("force:");
    Serial.print(force);
    Serial.print("\tmotor:");
    Serial.print(motor);    
    speedcontrol(force);
    Serial.print("\tm:");
    Serial.print(motor);
    Serial.print("\tD:");
    Serial.print(force + DISTANCE);
    Serial.print("\tStop:");
    Serial.print(STOP_FORCE);
    Serial.print("\tt:");
    Serial.println(t);
    t = actual_time;
  }
}

void speedcontrol(unsigned int f) {
  
    
    if (f >= STOP_FORCE) {
      r = 0.90 * motor;
      motor = (int)r;
    }
    else {
      d = f + DISTANCE;
      if (abs(d - STOP_FORCE) < VARIATION_ALLOWED ) {
        // do nothing
        motor++;
      }
      else if ( d < STOP_FORCE) {
        if (motor < MOTOR_TURNOFF) motor = MOTOR_TURNOFF;
        motor = motor + RATE_UP;
      }
      else if (d > STOP_FORCE) {
        if (motor > MOTOR_TURNOFF){
          motor = motor - RATE_DOWN;
        }
      }
      else motor++;
    }
      
    if (motor > MAX_LEVEL) motor = MAX_LEVEL;
    if (motor < MOTOR_TURNOFF ) motor = 0;    
    //if ( motor != lastlevel) {
      //p = map(motor, 0, 1023, 0, 255);         
      //SoftPWMSet(MOTORPIN, p); 
      SoftPWMSetPercent(MOTORPIN, motor);
      //analogWrite(MOTORPIN, p);     
    //}
    lastlevel = motor;
  
}

int readlevel(byte readtimes) {
  val = 0;
  for (byte i = 0; i < readtimes; i++) {
    val += analogRead(IRPIN);
    delay(1);
  }
  r = (val / readtimes) * SENSITIVITY;
  if (r < 0) r = 0;
  return map((int)r, 0, 1023, 0, 255);  
  //return int(r);
}

bool time_is_over(uint32_t wait_time, uint32_t start_time) {

  if ( (actual_time - start_time) > wait_time) return true;
  return false;

}

