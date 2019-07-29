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

uint8_t CSRD::getMessageBuffer(uint8_t *mbuffer){
    memcpy(mbuffer, this->buffer, messageLength);
    return messageLength;
}

uint8_t CSRD::createInitialRegisterMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t status,uint8_t val0,uint8_t val1,uint8_t val2){   
    messageLength = 8;
    buffer[0]=RP_STATUS;
    buffer[1]=RP_INITIALREG;
    buffer[2]=lowByte(nodeid);
    buffer[3]=lowByte(nodeid);
    buffer[4]=status;
    buffer[5]=val0;
    buffer[6]=val1;
    buffer[7]=val2;    
    return messageLength;
    //logger->debug("create Initial Register Message");        
}

uint8_t CSRD::createStatusMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t status){    
    messageLength = 6;
    buffer[0]=RP_STATUS;
    buffer[1]=RP_REPORT_STATUS;
    buffer[2]=highByte(nodeid);
    buffer[3]=lowByte(nodeid);
    buffer[4]=status;
    buffer[5]=serverAddr;
    buffer[6]=0;
    buffer[7]=0;    
    return messageLength;
}

uint8_t CSRD::createACKMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t status){    
    messageLength = 6;
    buffer[0]=RP_STATUS;
    buffer[1]=RP_REPORT_ACK;
    buffer[2]=highByte(nodeid);
    buffer[3]=lowByte(nodeid);
    buffer[4]=element;
    buffer[5]=status;
    buffer[6]=0;
    buffer[7]=0;    
    return messageLength;
}

uint8_t CSRD::createBroadcastOPMessage(uint8_t group,uint8_t element,uint8_t state,uint8_t val0,uint8_t val1,uint8_t val2){    
    messageLength = 8;
    buffer[0]=RP_BROADCAST;
    buffer[1]=RP_OPERATION;
    buffer[2]=group;
    buffer[3]=element;
    buffer[4]=state;
    buffer[5]=val0;
    buffer[6]=val1;
    buffer[7]=val2;    
    return messageLength;

}

uint8_t CSRD::createBroadcastActionMessage(uint8_t group,uint8_t element,uint8_t action,uint8_t val0,uint8_t val1,uint8_t val2){
    messageLength = 8;
    buffer[0]=RP_BROADCAST;
    buffer[1]=RP_ACTION;
    buffer[2]=group;
    buffer[3]=element;
    buffer[4]=action;
    buffer[5]=val0;
    buffer[6]=val1;
    buffer[7]=val2;    
    return messageLength;
}

uint8_t CSRD::createBroadcastRequestRegister(uint8_t group){    
    messageLength = 5;
    buffer[0]=RP_BROADCAST;
    buffer[1]=RP_ACTION;
    buffer[2]=group;
    buffer[3]=0xff;
    buffer[4]=RP_AC_REGISTER;
    buffer[5]=0;
    buffer[6]=0;
    buffer[7]=0;    
    return messageLength;
}

uint8_t CSRD::createBroadcastWriteMessage(uint8_t group,uint8_t element,uint8_t param_idx,uint8_t val0,uint8_t val1,uint8_t val2){    
    messageLength = 8;
    buffer[0] = RP_BROADCAST;
    buffer[1] = RP_WRITE;
    buffer[2] = group;
    buffer[3] = element;
    buffer[4] = param_idx;
    buffer[5] = val0;
    buffer[6] = val1;
    buffer[7] = val2;    
    return messageLength;
}
uint8_t CSRD::createAddressedWriteMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t param_idx,uint8_t val0,uint8_t val1){    
    messageLength = 8;
    buffer[0] = RP_ADDRESSED;
    buffer[1] = RP_WRITE;
    buffer[2] = highByte(nodeid);
    buffer[3] = lowByte(nodeid);
    buffer[4] = element;
    buffer[5] = param_idx;
    buffer[6] = val0;
    buffer[7] = val1;    
    return messageLength;
}
uint8_t CSRD::createAddressedReadMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t param_idx){    
    messageLength = 6;
    buffer[0] = RP_ADDRESSED;
    buffer[1] = RP_READ;
    buffer[2] = highByte(nodeid);
    buffer[3] = lowByte(nodeid);
    buffer[4] = element;
    buffer[5] = param_idx;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}
uint8_t CSRD::createAddressedOPMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t state,uint8_t val0,uint8_t val1){    
    messageLength = 8;
    buffer[0] = RP_ADDRESSED;
    buffer[1] = RP_OPERATION;
    buffer[2] = highByte(nodeid);
    buffer[3] = lowByte(nodeid);
    buffer[4] = element;
    buffer[5] = state;
    buffer[6] = val0;
    buffer[7] = val1;    
    return messageLength;    
}

uint8_t CSRD::createAddressedActionMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t action,uint8_t val0,uint8_t val1){    
    messageLength = 8;
    buffer[0] = RP_ADDRESSED;
    buffer[1] = RP_ACTION;
    buffer[2] = highByte(nodeid);
    buffer[3] = lowByte(nodeid);
    buffer[4] = element;
    buffer[5] = action;
    buffer[6] = val0;
    buffer[7] = val1;    
    return messageLength;    
}

uint8_t CSRD::createAddressedStatusMessage(uint8_t status_code, uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t p0,uint8_t p1,uint8_t p2){    
    messageLength = 8;
    buffer[0] = RP_STATUS;
    buffer[1] = status_code;
    buffer[2] = highByte(nodeid);
    buffer[3] = lowByte(nodeid);
    buffer[4] = element;
    buffer[5] = p0;
    buffer[6] = p1;
    buffer[7] = p2;    
    return messageLength;
}

uint8_t CSRD::createEmergencyBroadcast(uint8_t group){    
    messageLength = 5;
    buffer[0] = RP_BROADCAST;
    buffer[1] = RP_OPERATION;
    buffer[2] = group;
    buffer[3] = 255;
    buffer[4] = EMERGENCY;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}
uint8_t CSRD::createEmergency(uint8_t serverAddr,uint16_t nodeid){    
    messageLength = 6;
    buffer[0] = RP_ADDRESSED;
    buffer[1] = RP_OPERATION;
    buffer[2] = highByte(nodeid);
    buffer[3] = lowByte(nodeid);
    buffer[4] = 255;
    buffer[5] = EMERGENCY;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createBackToNormalBroadcast(uint8_t group){    
    messageLength = 5;
    buffer[0] = RP_BROADCAST;
    buffer[1] = RP_OPERATION;
    buffer[2] = group;
    buffer[3] = 255;
    buffer[4] = NORMAL;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}
uint8_t CSRD::createBackToNormal(uint8_t serverAddr,uint16_t nodeid){    
    messageLength = 6;
    buffer[0] = RP_ADDRESSED;
    buffer[1] = RP_OPERATION;
    buffer[2] = highByte(nodeid);
    buffer[3] = lowByte(nodeid);
    buffer[4] = 255;
    buffer[5] = NORMAL;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createLowBattery(uint8_t serverAddr,uint16_t nodeid){    
    messageLength = 7;
    buffer[0] = RP_ADDRESSED;
    buffer[1] = RP_ACTION;
    buffer[2] = highByte(nodeid);
    buffer[3] = lowByte(nodeid);
    buffer[4] = 255;
    buffer[5] = AC_LOWBATTERY;
    buffer[6] = serverAddr;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createRestoreDefaultConfig(uint8_t serverAddr,uint16_t nodeid,uint8_t nodeAddr){    
    messageLength = 7;
    buffer[0] = RP_ADDRESSED;
    buffer[1] = RP_ACTION;
    buffer[2] = highByte(nodeid);
    buffer[3] = lowByte(nodeid);
    buffer[4] = 255;
    buffer[5] = AC_RESTORE_DEFAULT_PARAMS;
    buffer[6] = serverAddr;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createServerAutoEnum(uint16_t nodeid){    
    messageLength = 3;
    buffer[0] = BR_SERVER_AUTO_ENUM;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);    
    buffer[3] = 0;
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createCarAutoEnum(uint16_t nodeid){    
    messageLength = 3;
    buffer[0] = BR_CAR_AUTO_ENUM;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);
    buffer[3] = 0;
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createNodeId(uint16_t nodeid){    
    messageLength = 3;
    buffer[0] = RP_ID_RESOLUTION;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);
    buffer[3] = 0;
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createRCId(uint16_t nodeid){    
    messageLength = 3;
    buffer[0] = RC_ID;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);
    buffer[3] = 0;
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createCarId(uint16_t nodeid){    
    messageLength = 3;
    buffer[0] = CAR_ID;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);
    buffer[3] = 0;
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createAcquire(uint16_t nodeid, uint8_t serverid){    
    messageLength = 4;
    buffer[0] = CAR_ACQUIRE;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);
    buffer[3] = serverid;    
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createAcquireAck(uint16_t nodeid, uint8_t serverid){    
    messageLength = 4;
    buffer[0] = CAR_ACQUIRE_ACK;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);
    buffer[3] = serverid; 
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createAcquireNAck(uint16_t nodeid, uint8_t serverid){    
    messageLength = 4;
    buffer[0] = CAR_ACQUIRE_NACK;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);
    buffer[3] = serverid; 
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createCarRelease(uint16_t nodeid, uint8_t serverid){    
    messageLength = 4;
    buffer[0] = CAR_RELEASE;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);
    buffer[3] = serverid;    
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createCarReleaseAck(uint16_t nodeid, uint8_t serverid){    
    messageLength = 4;
    buffer[0] = CAR_RELEASE_ACK;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);
    buffer[3] = serverid;    
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createRCCarRegister(uint16_t nodeid, uint8_t serverid){    
    messageLength = 4;
    buffer[0] = RC_CAR_REGISTER;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);
    buffer[3] = serverid;    
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createRCCarRegisterAck(uint16_t nodeid, uint8_t serverid){    
    messageLength = 4;
    buffer[0] = RC_CAR_REGISTER_ACK;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);
    buffer[3] = serverid;    
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createCarKeepAlive(uint16_t nodeid, uint8_t serverid){    
    messageLength = 4;
    buffer[0] = CAR_KEEP_ALIVE;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);
    buffer[3] = serverid;    
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createRCKeepAlive(uint16_t nodeid, uint8_t serverid){    
    messageLength = 4;
    buffer[0] = RC_KEEP_ALIVE;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);
    buffer[3] = serverid;    
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createCarLightOnOff(uint16_t nodeid, uint8_t serverid){    
    messageLength = 4;
    buffer[0] = RC_LIGHTS;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);
    buffer[3] = serverid;    
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createCarBreakLightOnOff(uint16_t nodeid, uint8_t serverid){    
    messageLength = 4;
    buffer[0] = RC_BREAK_LIGHTS;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);
    buffer[3] = serverid;    
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createStopCar(uint16_t nodeid, uint8_t serverid){    
    messageLength = 4;
    buffer[0] = RC_STOP_CAR;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);
    buffer[3] = serverid;    
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createRCMove(uint16_t nodeid, uint8_t serverid,uint8_t speed, uint8_t direction){    
    messageLength = 6;
    buffer[0] = RC_MOVE;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);
    buffer[3] = serverid;
    buffer[4] = speed;
    buffer[5] = direction;    
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createRCTurn(uint16_t nodeid, uint8_t serverid,uint8_t angle, uint8_t direction){    
    messageLength = 6;
    buffer[0] = RC_TURN;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);
    buffer[3] = serverid;
    buffer[4] = angle;
    buffer[5] = direction;    
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
}

uint8_t CSRD::createSaveParam(uint16_t nodeid, uint8_t serverid, uint8_t idx, uint8_t value){    
    messageLength = 6;
    buffer[0] = SAVE_PARAM;
    buffer[1] = highByte(nodeid);
    buffer[2] = lowByte(nodeid);
    buffer[3] = serverid;
    buffer[4] = idx;
    buffer[5] = value;    
    buffer[6] = 0;
    buffer[7] = 0;    
    return messageLength;
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

void CSRD::dumpBuffer(){
    logger->debug("CSRD buffer: %X %X %X %X %X %X %X %X",
    buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);    
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

string CSRD::bufferToJson(){
    return nullptr;
}