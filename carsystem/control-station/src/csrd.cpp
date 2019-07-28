#include "csrd.h"

uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

CSRD::CSRD(log4cpp::Category *logger){
    this->logger = logger;    
    //resetToDefault();
}

CSRD::CSRD(log4cpp::Category *logger, uint16_t radioID, uint8_t *mbuffer, uint8_t mbuffer_size) : CSRD(logger){    
    setMessage(radioID, mbuffer, mbuffer_size);    
}

uint8_t CSRD::setMessage(uint16_t radioID, uint8_t *mbuffer, uint8_t mbuffer_size){
    uint8_t s = MESSAGE_SIZE;

    if (mbuffer_size < MESSAGE_SIZE) s = mbuffer_size;
   
    memset(buffer, '\0', MESSAGE_SIZE);
    memcpy(buffer, mbuffer, s);
    messageLength = s;
    this->radioID = radioID;
    time(&time_received);
    return s;
}

uint8_t CSRD::getRadioMessageBuffer(uint8_t *mbuffer){    
    memcpy(mbuffer, this->radioBuffer, radioMessageLength);
    return radioMessageLength;    
}

uint8_t CSRD::getMessageBuffer(uint8_t *mbuffer){
    memcpy(mbuffer, this->buffer, messageLength);
    return messageLength;
}

uint8_t CSRD::createInitialRegisterMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t status,uint8_t val0,uint8_t val1,uint8_t val2){   
    radioMessageLength = 8;
    radioBuffer[0]=RP_STATUS;
    radioBuffer[1]=RP_INITIALREG;
    radioBuffer[2]=lowByte(nodeid);
    radioBuffer[3]=lowByte(nodeid);
    radioBuffer[4]=status;
    radioBuffer[5]=val0;
    radioBuffer[6]=val1;
    radioBuffer[7]=val2;    
    return radioMessageLength;
    //logger->debug("create Initial Register Message");        
}

uint8_t CSRD::createStatusMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t status){    
    radioMessageLength = 6;
    radioBuffer[0]=RP_STATUS;
    radioBuffer[1]=RP_REPORT_STATUS;
    radioBuffer[2]=highByte(nodeid);
    radioBuffer[3]=lowByte(nodeid);
    radioBuffer[4]=status;
    radioBuffer[5]=serverAddr;
    radioBuffer[6]=0;
    radioBuffer[7]=0;    
    return radioMessageLength;
}

uint8_t CSRD::createACKMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t status){    
    radioMessageLength = 6;
    radioBuffer[0]=RP_STATUS;
    radioBuffer[1]=RP_REPORT_ACK;
    radioBuffer[2]=highByte(nodeid);
    radioBuffer[3]=lowByte(nodeid);
    radioBuffer[4]=element;
    radioBuffer[5]=status;
    radioBuffer[6]=0;
    radioBuffer[7]=0;    
    return radioMessageLength;
}

uint8_t CSRD::createBroadcastOPMessage(uint8_t group,uint8_t element,uint8_t state,uint8_t val0,uint8_t val1,uint8_t val2){    
    radioMessageLength = 8;
    radioBuffer[0]=RP_BROADCAST;
    radioBuffer[1]=RP_OPERATION;
    radioBuffer[2]=group;
    radioBuffer[3]=element;
    radioBuffer[4]=state;
    radioBuffer[5]=val0;
    radioBuffer[6]=val1;
    radioBuffer[7]=val2;    
    return radioMessageLength;

}

uint8_t CSRD::createBroadcastActionMessage(uint8_t group,uint8_t element,uint8_t action,uint8_t val0,uint8_t val1,uint8_t val2){
    radioMessageLength = 8;
    radioBuffer[0]=RP_BROADCAST;
    radioBuffer[1]=RP_ACTION;
    radioBuffer[2]=group;
    radioBuffer[3]=element;
    radioBuffer[4]=action;
    radioBuffer[5]=val0;
    radioBuffer[6]=val1;
    radioBuffer[7]=val2;    
    return radioMessageLength;
}

uint8_t CSRD::createBroadcastRequestRegister(uint8_t group){    
    radioMessageLength = 5;
    radioBuffer[0]=RP_BROADCAST;
    radioBuffer[1]=RP_ACTION;
    radioBuffer[2]=group;
    radioBuffer[3]=0xff;
    radioBuffer[4]=RP_AC_REGISTER;
    radioBuffer[5]=0;
    radioBuffer[6]=0;
    radioBuffer[7]=0;    
    return radioMessageLength;
}

uint8_t CSRD::createBroadcastWriteMessage(uint8_t group,uint8_t element,uint8_t param_idx,uint8_t val0,uint8_t val1,uint8_t val2){    
    radioMessageLength = 8;
    radioBuffer[0] = RP_BROADCAST;
    radioBuffer[1] = RP_WRITE;
    radioBuffer[2] = group;
    radioBuffer[3] = element;
    radioBuffer[4] = param_idx;
    radioBuffer[5] = val0;
    radioBuffer[6] = val1;
    radioBuffer[7] = val2;    
    return radioMessageLength;
}
uint8_t CSRD::createAddressedWriteMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t param_idx,uint8_t val0,uint8_t val1){    
    radioMessageLength = 8;
    radioBuffer[0] = RP_ADDRESSED;
    radioBuffer[1] = RP_WRITE;
    radioBuffer[2] = highByte(nodeid);
    radioBuffer[3] = lowByte(nodeid);
    radioBuffer[4] = element;
    radioBuffer[5] = param_idx;
    radioBuffer[6] = val0;
    radioBuffer[7] = val1;    
    return radioMessageLength;
}
uint8_t CSRD::createAddressedReadMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t param_idx){    
    radioMessageLength = 6;
    radioBuffer[0] = RP_ADDRESSED;
    radioBuffer[1] = RP_READ;
    radioBuffer[2] = highByte(nodeid);
    radioBuffer[3] = lowByte(nodeid);
    radioBuffer[4] = element;
    radioBuffer[5] = param_idx;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}
uint8_t CSRD::createAddressedOPMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t state,uint8_t val0,uint8_t val1){    
    radioMessageLength = 8;
    radioBuffer[0] = RP_ADDRESSED;
    radioBuffer[1] = RP_OPERATION;
    radioBuffer[2] = highByte(nodeid);
    radioBuffer[3] = lowByte(nodeid);
    radioBuffer[4] = element;
    radioBuffer[5] = state;
    radioBuffer[6] = val0;
    radioBuffer[7] = val1;    
    return radioMessageLength;    
}

uint8_t CSRD::createAddressedActionMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t action,uint8_t val0,uint8_t val1){    
    radioMessageLength = 8;
    radioBuffer[0] = RP_ADDRESSED;
    radioBuffer[1] = RP_ACTION;
    radioBuffer[2] = highByte(nodeid);
    radioBuffer[3] = lowByte(nodeid);
    radioBuffer[4] = element;
    radioBuffer[5] = action;
    radioBuffer[6] = val0;
    radioBuffer[7] = val1;    
    return radioMessageLength;    
}

uint8_t CSRD::createAddressedStatusMessage(uint8_t status_code, uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t p0,uint8_t p1,uint8_t p2){    
    radioMessageLength = 8;
    radioBuffer[0] = RP_STATUS;
    radioBuffer[1] = status_code;
    radioBuffer[2] = highByte(nodeid);
    radioBuffer[3] = lowByte(nodeid);
    radioBuffer[4] = element;
    radioBuffer[5] = p0;
    radioBuffer[6] = p1;
    radioBuffer[7] = p2;    
    return radioMessageLength;
}

uint8_t CSRD::createEmergencyBroadcast(uint8_t group){    
    radioMessageLength = 5;
    radioBuffer[0] = RP_BROADCAST;
    radioBuffer[1] = RP_OPERATION;
    radioBuffer[2] = group;
    radioBuffer[3] = 255;
    radioBuffer[4] = EMERGENCY;
    radioBuffer[5] = 0;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}
uint8_t CSRD::createEmergency(uint8_t serverAddr,uint16_t nodeid){    
    radioMessageLength = 6;
    radioBuffer[0] = RP_ADDRESSED;
    radioBuffer[1] = RP_OPERATION;
    radioBuffer[2] = highByte(nodeid);
    radioBuffer[3] = lowByte(nodeid);
    radioBuffer[4] = 255;
    radioBuffer[5] = EMERGENCY;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createBackToNormalBroadcast(uint8_t group){    
    radioMessageLength = 5;
    radioBuffer[0] = RP_BROADCAST;
    radioBuffer[1] = RP_OPERATION;
    radioBuffer[2] = group;
    radioBuffer[3] = 255;
    radioBuffer[4] = NORMAL;
    radioBuffer[5] = 0;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}
uint8_t CSRD::createBackToNormal(uint8_t serverAddr,uint16_t nodeid){    
    radioMessageLength = 6;
    radioBuffer[0] = RP_ADDRESSED;
    radioBuffer[1] = RP_OPERATION;
    radioBuffer[2] = highByte(nodeid);
    radioBuffer[3] = lowByte(nodeid);
    radioBuffer[4] = 255;
    radioBuffer[5] = NORMAL;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createLowBattery(uint8_t serverAddr,uint16_t nodeid){    
    radioMessageLength = 7;
    radioBuffer[0] = RP_ADDRESSED;
    radioBuffer[1] = RP_ACTION;
    radioBuffer[2] = highByte(nodeid);
    radioBuffer[3] = lowByte(nodeid);
    radioBuffer[4] = 255;
    radioBuffer[5] = AC_LOWBATTERY;
    radioBuffer[6] = serverAddr;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createRestoreDefaultConfig(uint8_t serverAddr,uint16_t nodeid,uint8_t nodeAddr){    
    radioMessageLength = 7;
    radioBuffer[0] = RP_ADDRESSED;
    radioBuffer[1] = RP_ACTION;
    radioBuffer[2] = highByte(nodeid);
    radioBuffer[3] = lowByte(nodeid);
    radioBuffer[4] = 255;
    radioBuffer[5] = AC_RESTORE_DEFAULT_PARAMS;
    radioBuffer[6] = serverAddr;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createServerAutoEnum(uint16_t nodeid){    
    radioMessageLength = 3;
    radioBuffer[0] = BR_SERVER_AUTO_ENUM;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);    
    radioBuffer[3] = 0;
    radioBuffer[4] = 0;
    radioBuffer[5] = 0;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createCarAutoEnum(uint16_t nodeid){    
    radioMessageLength = 3;
    radioBuffer[0] = BR_CAR_AUTO_ENUM;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);
    radioBuffer[3] = 0;
    radioBuffer[4] = 0;
    radioBuffer[5] = 0;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createNodeId(uint16_t nodeid){    
    radioMessageLength = 3;
    radioBuffer[0] = RP_ID_RESOLUTION;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);
    radioBuffer[3] = 0;
    radioBuffer[4] = 0;
    radioBuffer[5] = 0;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createRCId(uint16_t nodeid){    
    radioMessageLength = 3;
    radioBuffer[0] = RC_ID;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);
    radioBuffer[3] = 0;
    radioBuffer[4] = 0;
    radioBuffer[5] = 0;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createCarId(uint16_t nodeid){    
    radioMessageLength = 3;
    radioBuffer[0] = CAR_ID;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);
    radioBuffer[3] = 0;
    radioBuffer[4] = 0;
    radioBuffer[5] = 0;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createAcquire(uint16_t nodeid, uint8_t serverid){    
    radioMessageLength = 4;
    radioBuffer[0] = CAR_ACQUIRE;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);
    radioBuffer[3] = serverid;    
    radioBuffer[4] = 0;
    radioBuffer[5] = 0;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createAcquireAck(uint16_t nodeid, uint8_t serverid){    
    radioMessageLength = 4;
    radioBuffer[0] = CAR_ACQUIRE_ACK;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);
    radioBuffer[3] = serverid; 
    radioBuffer[4] = 0;
    radioBuffer[5] = 0;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createAcquireNAck(uint16_t nodeid, uint8_t serverid){    
    radioMessageLength = 4;
    radioBuffer[0] = CAR_ACQUIRE_NACK;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);
    radioBuffer[3] = serverid; 
    radioBuffer[4] = 0;
    radioBuffer[5] = 0;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createCarRelease(uint16_t nodeid, uint8_t serverid){    
    radioMessageLength = 4;
    radioBuffer[0] = CAR_RELEASE;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);
    radioBuffer[3] = serverid;    
    radioBuffer[4] = 0;
    radioBuffer[5] = 0;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createCarReleaseAck(uint16_t nodeid, uint8_t serverid){    
    radioMessageLength = 4;
    radioBuffer[0] = CAR_RELEASE_ACK;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);
    radioBuffer[3] = serverid;    
    radioBuffer[4] = 0;
    radioBuffer[5] = 0;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createRCCarRegister(uint16_t nodeid, uint8_t serverid){    
    radioMessageLength = 4;
    radioBuffer[0] = RC_CAR_REGISTER;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);
    radioBuffer[3] = serverid;    
    radioBuffer[4] = 0;
    radioBuffer[5] = 0;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createRCCarRegisterAck(uint16_t nodeid, uint8_t serverid){    
    radioMessageLength = 4;
    radioBuffer[0] = RC_CAR_REGISTER_ACK;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);
    radioBuffer[3] = serverid;    
    radioBuffer[4] = 0;
    radioBuffer[5] = 0;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createCarKeepAlive(uint16_t nodeid, uint8_t serverid){    
    radioMessageLength = 4;
    radioBuffer[0] = CAR_KEEP_ALIVE;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);
    radioBuffer[3] = serverid;    
    radioBuffer[4] = 0;
    radioBuffer[5] = 0;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createRCKeepAlive(uint16_t nodeid, uint8_t serverid){    
    radioMessageLength = 4;
    radioBuffer[0] = RC_KEEP_ALIVE;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);
    radioBuffer[3] = serverid;    
    radioBuffer[4] = 0;
    radioBuffer[5] = 0;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createCarLightOnOff(uint16_t nodeid, uint8_t serverid){    
    radioMessageLength = 4;
    radioBuffer[0] = RC_LIGHTS;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);
    radioBuffer[3] = serverid;    
    radioBuffer[4] = 0;
    radioBuffer[5] = 0;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createCarBreakLightOnOff(uint16_t nodeid, uint8_t serverid){    
    radioMessageLength = 4;
    radioBuffer[0] = RC_BREAK_LIGHTS;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);
    radioBuffer[3] = serverid;    
    radioBuffer[4] = 0;
    radioBuffer[5] = 0;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createStopCar(uint16_t nodeid, uint8_t serverid){    
    radioMessageLength = 4;
    radioBuffer[0] = RC_STOP_CAR;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);
    radioBuffer[3] = serverid;    
    radioBuffer[4] = 0;
    radioBuffer[5] = 0;
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createRCMove(uint16_t nodeid, uint8_t serverid,uint8_t speed, uint8_t direction){    
    radioMessageLength = 6;
    radioBuffer[0] = RC_MOVE;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);
    radioBuffer[3] = serverid;
    radioBuffer[4] = speed;
    radioBuffer[5] = direction;    
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createRCTurn(uint16_t nodeid, uint8_t serverid,uint8_t angle, uint8_t direction){    
    radioMessageLength = 6;
    radioBuffer[0] = RC_TURN;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);
    radioBuffer[3] = serverid;
    radioBuffer[4] = angle;
    radioBuffer[5] = direction;    
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint8_t CSRD::createSaveParam(uint16_t nodeid, uint8_t serverid, uint8_t idx, uint8_t value){    
    radioMessageLength = 6;
    radioBuffer[0] = SAVE_PARAM;
    radioBuffer[1] = highByte(nodeid);
    radioBuffer[2] = lowByte(nodeid);
    radioBuffer[3] = serverid;
    radioBuffer[4] = idx;
    radioBuffer[5] = value;    
    radioBuffer[6] = 0;
    radioBuffer[7] = 0;    
    return radioMessageLength;
}

uint16_t CSRD::getNodeId(){   
   return word(buffer[1],buffer[2]);
}

uint16_t CSRD::getServerId(){
   return buffer[3];
}

uint8_t CSRD::getByte(uint8_t idx){
   if (idx >= MESSAGE_SIZE ) return 0;

   return buffer[idx];
}

uint8_t CSRD::getAngle(){
   return buffer[4];
}

uint8_t CSRD::getSpeed(){
   return buffer[4];
}

uint8_t CSRD::getDirection(){
   return buffer[5];
}

bool CSRD::isBroadcast(){
    if (buffer[0] == RP_BROADCAST){
        return true;
    }
    return false;
}

bool CSRD::isAddressed(){
    if (buffer[0] == RP_ADDRESSED){
        return true;
    }
    return false;
}

bool CSRD::isResolutionId(){
    return (buffer[0] == RP_ID_RESOLUTION);
}

bool CSRD::isServerAutoEnum(){
    return (buffer[0] == BR_SERVER_AUTO_ENUM);
}

bool CSRD::isCarAutoEnum(){
    return (buffer[0] == BR_CAR_AUTO_ENUM);
}

bool CSRD::isCarId(){
    return (buffer[0] == CAR_ID);
}

bool CSRD::isRCId(){
    return (buffer[0] == RC_ID);
}

bool CSRD::isAcquire(){
    return (buffer[0] == CAR_ACQUIRE);
}

bool CSRD::isAcquireAck(){
    return (buffer[0] == CAR_ACQUIRE_ACK);
}

bool CSRD::isRCCarRegister(){
    return (buffer[0] == RC_CAR_REGISTER);
}

bool CSRD::isRCCarRegisterAck(){
    return (buffer[0] == RC_CAR_REGISTER_ACK);
}


bool CSRD::isAcquireNAck(){
    return (buffer[0] == CAR_ACQUIRE_NACK);
}

bool CSRD::isCarRelease(){
    return (buffer[0] == CAR_RELEASE);
}

bool CSRD::isCarReleaseAck(){
    return (buffer[0] == CAR_RELEASE_ACK);
}

bool CSRD::isCarKeepAlive(){
    return (buffer[0] == CAR_KEEP_ALIVE);
}

bool CSRD::isRCKeepAlive(){
    return (buffer[0] == RC_KEEP_ALIVE);
}

bool CSRD::isRCBreakLights(){
    return (buffer[0] == RC_BREAK_LIGHTS);
}

bool CSRD::isRCLights(){
    return (buffer[0] == RC_LIGHTS);
}

bool CSRD::isStopCar(){
    return (buffer[0] == RC_STOP_CAR);
}

bool CSRD::isRCMove(){
    return (buffer[0] == RC_MOVE);
}

bool CSRD::isRCTurn(){
    return (buffer[0] == RC_TURN);
}

bool CSRD::isSaveParam(){
    return (buffer[0] == SAVE_PARAM);
}

bool CSRD::isStatus(){
    return (buffer[0] == RP_STATUS);        
}

bool CSRD::isOperation(){
    return (buffer[1] == RP_OPERATION);        
}

bool CSRD::isAction(){
    return (buffer[1] == RP_ACTION);        
}

bool CSRD::isRead(){
    if (isBroadcast()){
        return false;
    }

    return (buffer[1] == RP_READ);        
}

bool CSRD::isWrite(){
    return (buffer[1] == RP_WRITE);        
}

bool CSRD::isBroadcastRegister(){
    if (isBroadcast() && isAction() && buffer[4] == RP_AC_REGISTER){
        return true;
    }
    return false;
}

bool CSRD::isMyGroup(uint8_t mygroup){
    if (getGroup() == 0 || getGroup() == mygroup){
	return true;
    }
    return false;
}

bool CSRD::isLowBattery(uint8_t serveraddr){
   if ((getVal0() == serveraddr) && (getState() == AC_LOWBATTERY)){
      return true;
   }
   return false;
}

bool CSRD::isRestoreDefaultConfig(uint16_t myid){
   if (isAddressed() && isAction() && (getNodeNumber() == myid) && (getAction() == AC_RESTORE_DEFAULT_PARAMS)){
      return true;
   }
   return false;
}

uint8_t CSRD::getElement(){
    if (isAddressed() || isStatus()){
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

uint8_t CSRD::getStatusType(){
    if (!isStatus()){
        return RP_FILLER;
    }
   return buffer[1];
}

uint8_t CSRD::getStatus(){
    if (isBroadcast() || isAddressed()){
        return RP_FILLER;
    }
   return buffer[4];
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

uint8_t CSRD::getAction(){

    if (!isAction()){
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

uint8_t CSRD::getParamIdx(){
    if (isAddressed()){
        return buffer[5];
    }
    return buffer[4];
}

uint8_t CSRD::getVal0(){
    if (isAddressed() && !isStatus() ){
        return buffer[6];
    }
    return buffer[5];
}

uint8_t CSRD::getVal1(){
    if (isAddressed() && !isStatus() ){
        return buffer[7];
    }
    return buffer[6];
}

uint8_t CSRD::getVal2(){
    if (isAddressed() && !isStatus()){
        return RP_FILLER;
    }
    return buffer[7];
}

void CSRD::resetToDefault(){

    nodenumber=333;
    params[RP_PARAM_GROUP] = 1;
    params[RP_PARAM_BATTERY_THRESHOLD] = 30;
    params[RP_PARAM_SOUND_ON] = RP_OFF;
    params[RP_PARAM_SPEED_STEP] = 5;
    params[RP_PARAM_SPEED_COMPENSATION] = RP_ON;
    params[RP_PARAM_FREQUENCY] = 2; //0=433MHz 1=868MHz 2=915MHz
    params[RP_PARAM_BREAK_RATE] = 20; //0=433MHz 1=868MHz 2=915MHz
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
    case(7):
      return NIGHT;
      break;
    case(8):
      return DAY;
      break;
    case(9):
      return ALL_BLINKING;
      break;
    default:
      return OFF;
  }
}

string CSRD::bufferToHexString(){
    char tempbuf[50];
    memset(tempbuf, '\0', sizeof(tempbuf));
    sprintf(tempbuf,"%02X %02X %02X %02X %02X %02X %02X %02X",
                    buffer[0],
                    buffer[1],
                    buffer[2],
                    buffer[3],
                    buffer[4],
                    buffer[5],
                    buffer[6],
                    buffer[7]);
    string s(tempbuf);
    return s;
}

string CSRD::radioBufferToHexString(){
    char tempbuf[50];
    memset(tempbuf, '\0', sizeof(tempbuf));
    sprintf(tempbuf,"%02X %02X %02X %02X %02X %02X %02X %02X",
                    radioBuffer[0],
                    radioBuffer[1],
                    radioBuffer[2],
                    radioBuffer[3],
                    radioBuffer[4],
                    radioBuffer[5],
                    radioBuffer[6],
                    radioBuffer[7]);
    string s(tempbuf);
    return s;
}

void CSRD::dumpBuffer(){
    logger->debug("CSRD buffer: %X %X %X %X %X %X %X %X",
    buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);    
}

void CSRD::dumpRadioBuffer(){
    logger->debug("CSRD buffer: %X %X %X %X %X %X %X %X",
    radioBuffer[0], radioBuffer[1], radioBuffer[2], radioBuffer[3], radioBuffer[4], radioBuffer[5], radioBuffer[6], radioBuffer[7]);    
}

uint16_t CSRD::word(uint8_t a, uint8_t b){
    uint16_t combined = a << 8 | b;
    return combined;
}

uint8_t CSRD::lowByte(uint16_t a){
    uint8_t b = a & 0x00ff;
    return b;
}

uint8_t CSRD::highByte(uint16_t a){
    uint8_t b = a >> 8;
    return b;
}