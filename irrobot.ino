#include <IRremote.h>
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_PWMServoDriver.h"


#define AVANTI 0xE10
#define INDIETRO 0x910
#define STOP 0xA90

#define SINISTRA  0x610
#define DESTRA  0x110


#define HOLD 0
#define ACCELL 1
#define DECELL 2
#define RETURN_TO_ZERO 3
#define TURN_LEFT 4
#define TURN_RIGHT 5
#define RETURN_STRAIGHT 6

#define OP_DELAY 5
#define DV 4
#define DTQ 10

#define EXP_DECAY 127.f/128.f

Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
Adafruit_DCMotor *sx = AFMS.getMotor(3), *dx =AFMS.getMotor(1);
int sx_speed=0, dx_speed=0;     //TODO: creare libreria intorno adafruit motorshield
int tot_speed=0;

void accell(int val){
  int temp_sp = tot_speed+val;
  int abs_sp = constrain(abs(temp_sp), 0, 255);

  if(temp_sp >= 0 && tot_speed < 0){
    //changing direction. brake, reverse, accellerate
    sx->setSpeed(0); 
    dx->setSpeed(0);
    //TODO: add delay?
    sx->run(FORWARD);
    dx->run(FORWARD);
  }else if(temp_sp <0 && tot_speed >= 0){
    //changing direction. brake, reverse, accellerate
    sx->setSpeed(0); 
    dx->setSpeed(0);
    //TODO: add delay?
    sx->run(BACKWARD);
    dx->run(BACKWARD);
  }

  sx->setSpeed(abs_sp);
  dx->setSpeed(abs_sp);
  tot_speed = (temp_sp<0)? -abs_sp : abs_sp;

  sx_speed=dx_speed=tot_speed;
}

bool retToZero(){
  if(tot_speed == 0){
    return true;
  }

  float t_speed = tot_speed*EXP_DECAY;
  tot_speed = floor(t_speed);
  sx_speed=dx_speed=tot_speed;

  sx->setSpeed(abs(tot_speed));
  dx->setSpeed(abs(tot_speed));

  return false;
}

void applyTorque(int val){

}

IRrecv irrecv(11);

int status = HOLD;
int lastTime;

void setup(){
  Serial.begin(9600);

  irrecv.blink13(true);
  irrecv.enableIRIn(); // Start the receiver

  AFMS.begin();
  sx->run(FORWARD);
  dx->run(FORWARD);
  sx->setSpeed(0);
  dx->setSpeed(0);
  status=HOLD;
  lastTime=millis();

}

void loop() {
  decode_results results;

  if(irrecv.decode(&results)){
    Serial.println(results.value, HEX);
    irrecv.resume();

    switch(results.value){
      case AVANTI:
        status = ACCELL;
        break;
      case INDIETRO:
        status = DECELL;
        break;
      case STOP:
        status = RETURN_TO_ZERO;
        break;
      case SINISTRA:
        status = TURN_LEFT;
        break;
      case DESTRA:
        status = TURN_RIGHT;
      default:
        status = HOLD;
        break;
    }
  }

  if(millis()-lastTime > OP_DELAY){
    switch(status){
      case ACCELL:
        accell(DV);
        status=HOLD;
        break;
      case DECELL:
        accell(-DV);
        status=HOLD;
        break;
      case RETURN_TO_ZERO:
        status= (retToZero())? HOLD : RETURN_TO_ZERO;
        break;
      case TURN_RIGHT:
        applyTorque(-DTQ);
        status = RETURN_STRAIGHT;
        break;
      case TURN_LEFT:
        applyTorque(DTQ);
        status = RETURN_STRAIGHT;
        break;
      case RETURN_STRAIGHT:
        status = HOLD;
        break;
      case HOLD:
        accell(0);
      default:
        break;
    }
    lastTime=millis();
  }

}
