#include "csrd.h"


CSRD::CSRD(){
    resetToDefault();
}

void CSRD::init(RH_RF69 *driver,RHReliableDatagram *manager,uint8_t *buf,int buffSize){
    this->driver=driver;
    this->manager=manager;
    radioBuffSize=buffSize;
    this->buf=buf;
}

void CSRD::sendMessage(char *buffer,uint16_t len){

}
uint16_t CSRD::getMessage(char *buffer){
    if (readMessage()){
        memcpy(buffer,this->buffer,MESSAGE_SIZE);
        return length;
    }
    return 0;

}


bool CSRD::readMessage(){

if (manager->available())
  {
    // Wait for a message addressed to us from the client

    uint8_t from;
    if (manager->recvfromAck(buf, &length, &from))
    {
        #ifndef CSRD_DEBUG
          Serial.print("got request from : 0x");
          Serial.print(from, HEX);
          Serial.print(": ");
          Serial.println((char*)buf);
        #endif // CSRD_DEBUG

        //we expect 8 bytes
        if (length>MESSAGE_SIZE){
            #ifndef CSRD_DEBUG
              Serial.print("message bigger than expected: ");
              Serial.println(len);
            #endif // CSRD_DEBUG
            return false;
        }
        memset(buffer,'\0',MESSAGE_SIZE);
        memcpy(buffer,buf,length);
        origin=from;
    }
  }

}

bool CSRD::isBroadcast(){
    if (buffer[0]==RP_BROADCAST){
        return true;
    }
    return false;
}

uint8_t CSRD::getGroup(){
    if (isOperation()||isBroadcast()){
        return buffer[2];
    }
}


bool CSRD::isAddressed(){
    if (buffer[0]==RP_ADDRESSED){
        return true;
    }
    return false;
}

uint16_t CSRD::getAddress(){
}

bool CSRD::isOperation(){

    if (buffer[1]==RP_OPERATION){
        return true;
    }
    return false;

}
uint8_t CSRD::getAction(){

    if (!isOperation()){
        return RP_FILLER;
    }

    if (isAddressed()){
        return buffer[4];
    }
    return buffer[3];
}
uint8_t CSRD::getValue(){

}

bool CSRD::isRead(){
    if (buffer[1]==RP_READ){
        return true;
    }
    return false;
}
uint8_t CSRD::getReadParam(){
}

bool CSRD::isWrite(){
    if (isBroadcast()){
        return false;
    }

    if (buffer[1]==RP_WRITE){
        return true;
    }
    return false;
}

uint8_t CSRD::getWriteParam(){
}
uint8_t CSRD::getWriteValue(){
}

void CSRD::resetToDefault(){
    nodenumber=333;
    params[RP_PARAM_GROUP]=1;
    params[RP_PARAM_BLINK_RATE]=20;
    params[RP_PARAM_BLINK_TIME_UNIT]=50;
    params[RP_PARAM_NORMAL_SPEED]=50;
    params[RP_PARAM_CURR_SPEED]=50;
    params[RP_PARAM_BATTERY_THRESHOLD]=30;
    params[RP_PARAM_SOUND_ON]=RP_OFF;
    params[RP_PARAM_SPEED_STEP]=5;
    params[RP_PARAM_SPEED_COMPENSATION]=RP_ON;
    params[RP_PARAM_FREQUENCY]=0; //0=433MHz 1=868MHz 2=915MHz
    params[RP_PARAM_BREAK_RATE]=20; //0=433MHz 1=868MHz 2=915MHz
}

void CSRD::saveDefaultToMemory(){
    uint8_t i;

    for (i=0;i<PARAMETERS_SIZE;i++){
    }

}
