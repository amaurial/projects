#include "csrd.h"


CSRD::CSR(){
}
void CSRD::init(){
}

void CSRD::sendMessage(char *buffer,uint16_t len){
}
uint16_t CSRD::getMessage(char *buffer){
}
bool CSRD::readMessage(){
}

bool CSRD::isBroadcast(){
    if (buffer[0]==RP_BROADCAST){
        return true;
    }
    return false;
}

uint8_t CSRD::getGroup(){
    if (isOperation()||isBroadcast()){
        return buffer[2]
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


