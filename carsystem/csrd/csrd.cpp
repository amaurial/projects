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
bool CSRD::sendInitialRegisterMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t status,uint8_t val0,uint8_t val1,uint8_t val2){
    uint8_t buf[MESSAGE_SIZE];
    buf[0]=RP_STATUS;
    buf[1]=RP_INITIALREG;
    buf[2]=highByte(nodeid);
    buf[3]=lowByte(nodeid);
    buf[4]=status;
    buf[5]=val0;
    buf[6]=val1;
    buf[7]=val2;
    return sendMessage(buf,MESSAGE_SIZE,serverAddr);
}

bool CSRD::sendBroadcastOPMessage(uint8_t serverAddr,uint8_t group,uint8_t element,uint8_t state,uint8_t val0,uint8_t val1,uint8_t val2){
    uint8_t buf[MESSAGE_SIZE];

    buf[0]=RP_BROADCAST;
    buf[1]=RP_OPERATION;
    buf[2]=group;
    buf[3]=element;
    buf[4]=state;
    buf[5]=val0;
    buf[6]=val1;
    buf[7]=val2;
    return sendMessage(buf,MESSAGE_SIZE,serverAddr);

}
bool CSRD::sendBroadcastWriteMessage(uint8_t serverAddr,uint8_t group,uint8_t element,uint8_t param_idx,uint8_t val0,uint8_t val1,uint8_t val2){
    uint8_t buf[MESSAGE_SIZE];
    buf[0]=RP_BROADCAST;
    buf[1]=RP_WRITE;
    buf[2]=group;
    buf[3]=element;
    buf[4]=param_idx;
    buf[5]=val0;
    buf[6]=val1;
    buf[7]=val2;
    return sendMessage(buf,MESSAGE_SIZE,serverAddr);
}
bool CSRD::sendAddressedWriteMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t param_idx,uint8_t val0,uint8_t val1){
    uint8_t buf[MESSAGE_SIZE];
    buf[0]=RP_ADDRESSED;
    buf[1]=RP_WRITE;
    buf[2]=highByte(nodeid);
    buf[3]=lowByte(nodeid);
    buf[4]=element;
    buf[5]=param_idx;
    buf[6]=val0;
    buf[7]=val1;
    return sendMessage(buf,MESSAGE_SIZE,serverAddr);
}
bool CSRD::sendAddressedReadMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t param_idx){
    uint8_t buf[MESSAGE_SIZE];
    buf[0]=RP_ADDRESSED;
    buf[1]=RP_READ;
    buf[2]=highByte(nodeid);
    buf[3]=lowByte(nodeid);
    buf[4]=element;
    buf[5]=param_idx;
    buf[6]=0;
    buf[7]=0;
    return sendMessage(buf,MESSAGE_SIZE,serverAddr);
}
bool CSRD::sendAddressedOPMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t state,uint8_t val0,uint8_t val1){
    uint8_t buf[MESSAGE_SIZE];
    buf[0]=RP_ADDRESSED;
    buf[1]=RP_OPERATION;
    buf[2]=highByte(nodeid);
    buf[3]=lowByte(nodeid);
    buf[4]=element;
    buf[5]=state;
    buf[6]=val0;
    buf[7]=val1;
    return sendMessage(buf,MESSAGE_SIZE,serverAddr);
}

bool CSRD::sendEmergencyBroadcast(uint8_t serverAddr,uint8_t group){
    uint8_t buf[MESSAGE_SIZE];
    buf[0]=RP_BROADCAST;
    buf[1]=RP_OPERATION;
    buf[2]=group;
    buf[3]=255;
    buf[4]=EMERGENCY;
    buf[5]=0;
    buf[6]=0;
    buf[7]=0;
    return sendMessage(buf,MESSAGE_SIZE,serverAddr);
}
bool CSRD::sendEmergency(uint8_t serverAddr,uint16_t nodeid){
    uint8_t buf[MESSAGE_SIZE];
    buf[0]=RP_ADDRESSED;
    buf[1]=RP_OPERATION;
    buf[2]=highByte(nodeid);
    buf[3]=lowByte(nodeid);
    buf[4]=255;
    buf[5]=EMERGENCY;
    buf[6]=0;
    buf[7]=0;
    return sendMessage(buf,MESSAGE_SIZE,serverAddr);
}

bool CSRD::sendBackToNormalBroadcast(uint8_t serverAddr,uint8_t group){
    uint8_t buf[MESSAGE_SIZE];
    buf[0]=RP_BROADCAST;
    buf[1]=RP_OPERATION;
    buf[2]=group;
    buf[3]=255;
    buf[4]=NORMAL;
    buf[5]=0;
    buf[6]=0;
    buf[7]=0;
    return sendMessage(buf,MESSAGE_SIZE,serverAddr);
}
bool CSRD::sendBackToNormal(uint8_t serverAddr,uint16_t nodeid){
    uint8_t buf[MESSAGE_SIZE];
    buf[0]=RP_ADDRESSED;
    buf[1]=RP_OPERATION;
    buf[2]=highByte(nodeid);
    buf[3]=lowByte(nodeid);
    buf[4]=255;
    buf[5]=NORMAL;
    buf[6]=0;
    buf[7]=0;
    return sendMessage(buf,MESSAGE_SIZE,serverAddr);
}

uint8_t CSRD::getMessage(uint8_t *mbuffer){
    if (readMessage()){
        memcpy(mbuffer,this->buffer,MESSAGE_SIZE);
        return length;
    }
    return 0;
}
uint8_t CSRD::getMessageBuffer(uint8_t *mbuffer){
    memcpy(mbuffer,this->buffer,MESSAGE_SIZE);
    return MESSAGE_SIZE;
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

bool CSRD::isAddressed(){
    if (buffer[0]==RP_ADDRESSED){
        return true;
    }
    return false;
}

bool CSRD::isStatus(){
    if (buffer[0]==RP_STATUS){
        return true;
    }
    return false;
}

bool CSRD::isOperation(){

    if (buffer[1]==RP_OPERATION){
        return true;
    }
    return false;
}

bool CSRD::isRead(){
    if (isBroadcast()){
        return false;
    }
    if (buffer[1]==RP_READ){
        return true;
    }
    return false;
}

bool CSRD::isWrite(){

    if (buffer[1]==RP_WRITE){
        return true;
    }
    return false;
}

uint8_t CSRD::getElement(){
    if (isAddressed()){
        return buffer[4];
    }
    return buffer[3];
}

uint16_t CSRD::getNodeNumber(){
    if (isBroadcast()){
        return RP_FILLER;
    }
   return word(buffer[2],buffer[3]);
}

uint8_t CSRD::getStatus(){
    if (isBroadcast() || isAddressed()){
        return RP_FILLER;
    }
   return buffer[5];
}

uint8_t CSRD::getState(){

    if (!isOperation()){
        return RP_FILLER;
    }
    if (isAddressed()){
        return buffer[5];
    }
    return buffer[4];
}

uint8_t CSRD::getGroup(){
    if (isBroadcast()){
        return buffer[2];
    }
    return RP_FILLER;
}

uint8_t CSRD::getVal0(){
    if (isAddressed()){
        return buffer[6];
    }
    return buffer[5];
}
uint8_t CSRD::getVal1(){
    if (isAddressed()){
        return buffer[7];
    }
    return buffer[6];
}
uint8_t CSRD::getVal2(){
    if (isAddressed()){
        return RP_FILLER;
    }
    return buffer[7];
}


void CSRD::resetToDefault(){
    nodenumber=333;
    params[RP_PARAM_GROUP]=1;
    params[RP_PARAM_BATTERY_THRESHOLD]=30;
    params[RP_PARAM_SOUND_ON]=RP_OFF;
    params[RP_PARAM_SPEED_STEP]=5;
    params[RP_PARAM_SPEED_COMPENSATION]=RP_ON;
    params[RP_PARAM_FREQUENCY]=0; //0=433MHz 1=868MHz 2=915MHz
    params[RP_PARAM_BREAK_RATE]=20; //0=433MHz 1=868MHz 2=915MHz
}
states CSRD::convertFromInt(uint8_t s){
  switch (s){
    case(0):
      return OFF;
      break;
    case(1):
      return ON;
      break;
    case(2):
      return STOPING;
      break;
    case(3):
      return ACCELERATING;
      break;
    case(4):
      return BLINKING;
      break;
    case(5):
      return EMERGENCY;
      break;
    case(6):
      return NORMAL;
      break;
    default:
      return OFF;

  }
}
