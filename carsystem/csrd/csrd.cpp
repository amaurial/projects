#include "csrd.h"

uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

CSRD::CSRD(){
    resetToDefault();
}

bool CSRD::init(RH_RF69 *driver,RHReliableDatagram *manager){

    #ifdef CSRD_DEBUG
        Serial.println("CSRD Initialization.");
    #endif // CSRD_DEBUG
    this->driver=driver;
    this->manager=manager;
    if (this->manager!=NULL){
        #ifdef CSRD_DEBUG
            Serial.println("Using manager.");
        #endif // CSRD_DEBUG
        if (!this->manager->init()){
        #ifdef CSRD_DEBUG
            Serial.println("Radio failed.");
        #endif // CSRD_DEBUG
            return false;
        }
    }
    else{
        #ifdef CSRD_DEBUG
            Serial.println("Using radio std.");
        #endif // CSRD_DEBUG
        if (!driver->init()){
            #ifdef CSRD_DEBUG
                Serial.println("Radio start failed.");
            #endif // CSRD_DEBUG
            return false;
        }
        if (!driver->setFrequency(915)){
            #ifdef CSRD_DEBUG
                Serial.println("Radio freq set failed.");
            #endif // CSRD_DEBUG
            return false;
        }

        driver->setEncryptionKey(key);
    }
    radioBuffSize=RH_RF69_MAX_MESSAGE_LEN;
    return true;
}

bool CSRD::sendMessage(uint8_t *sbuffer,uint8_t len,uint8_t serverAddr){
    if (this->manager!=NULL){
        return manager->sendtoWait(sbuffer, len, serverAddr);
    }else{
        driver->send(sbuffer, len);
        driver->waitPacketSent();
        return true;
    }
}

uint8_t CSRD::getMessage(uint8_t *mbuffer){
    if (readMessage()){
        memcpy(mbuffer,this->buffer,MESSAGE_SIZE);
        return length;
    }
    return 0;

}

bool CSRD::readMessage(){


    // Wait for a message addressed to us from the client
    if (isRadioOn())
    {
        uint8_t from;
        uint8_t len=radioBuffSize;

        if (this->manager!=NULL){

                if (this->manager->recvfromAck(buf, &len, &from))
                {
                    #ifdef CSRD_DEBUG
                      Serial.print("got request from : 0x");
                      Serial.print(from, HEX);
                      Serial.print(": ");
                      Serial.print(" size: ");
                      Serial.println(length);
                      Serial.println((char*)buf);
                    #endif // CSRD_DEBUG

                    //we expect 8 bytes
                    if (length>MESSAGE_SIZE){
                        #ifdef CSRD_DEBUG
                          Serial.print("message bigger than expected: ");
                          Serial.println(length);
                        #endif // CSRD_DEBUG
                        return false;
                    }
                    length=len;
                    if (length>0){
                        memset(buffer,'\0',MESSAGE_SIZE);
                        memcpy(buffer,buf,MESSAGE_SIZE);
                        origin=from;
                        return true;
                    }
                }
            }

        else {
            if (this->driver->recv(buf,&len)){
                length=len;
                if (length>0){
                    memset(buffer,'\0',MESSAGE_SIZE);
                    memcpy(buffer,buf,MESSAGE_SIZE);
                    origin=from;
                    return true;
                }
            }
        }

  }
    return false;
}

bool CSRD::isRadioOn(){
    #ifdef CSRD_DEBUG
        //Serial.print("radio status: ");
        //Serial.println(manager->available());
    #endif // CSRD_DEBUG
    if (manager!=NULL){
        return this->manager->available();
    }
    return this->driver->available();
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
