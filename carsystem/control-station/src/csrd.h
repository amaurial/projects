#ifndef __CSRD__H
#define __CSRD__H
#include <cstring>
#include <stdint.h>
#include <log4cpp/Category.hh>
#include "radioProtocol.h"

//#define CSRD_DEBUG 1

//states
enum states {
   OFF=0,
   ON,
   STOPING,
   ACCELERATING,
   BLINKING,
   EMERGENCY,
   NORMAL,
   NIGHT,
   DAY,
   ALL_BLINKING
};

enum ACTIONS{
   AC_LOWBATTERY=0,
   AC_RESTORE_DEFAULT_PARAMS,
   AC_SET_PARAM,
   AC_SET_SPEED,
   AC_ACQUIRE,
   AC_RELEASE,
   AC_ACK,
   AC_TURN, //val0 is the angle
   AC_MOVE, //val0 is the direction, val1 is the speed
   AC_FAIL
};

//car status
enum STATUS {
   ACTIVE=0,
   INACTIVE,
   CHARGING,
   PANNE,
   REGISTERED,
   NOT_REGISTERED,
   WAITING_REGISTRATION
};

//status type
enum STATUS_TYPE {
   STT_QUERY_STATUS=1,
   STT_ANSWER_STATUS,
   STT_QUERY_VALUE,
   STT_ANSWER_VALUE,
   STT_QUERY_VALUE_FAIL
};

//which part
enum objects_enum {
  LEFT_LIGHT=0,
  RIGHT_LIGHT,
  BREAK_LIGHT,
  REED,
  SIRENE_LIGHT,
  FRONT_LIGHT,
  MOTOR,
  IR_RECEIVE,
  IR_SEND,
  BOARD=9
};

class CSRD {

public:
    CSRD(log4cpp::Category *logger);    
    CSRD(log4cpp::Category *logger, uint16_t radioID, uint8_t *mbuffer, uint8_t mbuffer_size);

    uint8_t setMessage(uint16_t radioID, uint8_t *mbuffer, uint8_t mbuffer_size);    
    uint8_t getMessage(uint8_t *mbuffer);
    uint8_t getMessageBuffer(uint8_t *mbuffer);
    uint8_t getRadioMessageBuffer(uint8_t *mbuffer);
    uint8_t getMessageLength(){return messageLength;};
    uint8_t getRadioMessageLength(){return radioMessageLength;};
    uint16_t getRadioID(){return radioID;};
    
    bool isBroadcast();
    bool isAddressed();
    bool isStatus();
    bool isOperation();
    bool isAction();
    bool isRead();
    bool isWrite();
    bool isBroadcastRegister();
    bool isMyGroup(uint8_t mygroup);
    bool isLowBattery(uint8_t serverAddr);
    bool isRestoreDefaultConfig(uint16_t myid);
    bool isServerAutoEnum();
    bool isCarAutoEnum();
    bool isResolutionId();
    bool isRCId();
    bool isCarId();
    bool isAcquire();
    bool isAcquireAck();
	bool isRCCarRegister();
    bool isRCCarRegisterAck();
	bool isAcquireNAck();
    bool isCarRelease();
    bool isCarReleaseAck();
    bool isCarKeepAlive();
    bool isRCKeepAlive();
    bool isSaveParam();
    bool isRCLights();
    bool isRCBreakLights();
    bool isStopCar();
    bool isRCMove();
    bool isRCTurn();

    uint8_t getGroup();
    uint8_t getElement();
    uint8_t getState();
    uint8_t getStatus();
    uint8_t getParamIdx();
    uint8_t getVal0();
    uint8_t getVal1();
    uint8_t getVal2();
    uint16_t getNodeNumber();
    uint16_t getSender(){return origin;};
    uint8_t getAction();
    uint8_t getStatusType();
    uint16_t getNodeId();
	uint8_t getAngle();
	uint8_t getSpeed();
	uint8_t getDirection();
    uint16_t getServerId();
    uint8_t getByte(uint8_t idx);

    uint8_t createBroadcastOPMessage(uint8_t group,uint8_t element,uint8_t state,uint8_t val0,uint8_t val1,uint8_t val2);
    uint8_t createBroadcastRequestRegister(uint8_t group);
    uint8_t createBroadcastWriteMessage(uint8_t group,uint8_t element,uint8_t param_idx,uint8_t val0,uint8_t val1,uint8_t val2);
    uint8_t createBroadcastActionMessage(uint8_t group,uint8_t element,uint8_t action,uint8_t val0,uint8_t val1,uint8_t val2);

    uint8_t createAddressedWriteMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t param_idx,uint8_t val0,uint8_t val1);
    uint8_t createAddressedReadMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t param_idx);
    uint8_t createAddressedOPMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t state,uint8_t val0,uint8_t val1);
    uint8_t createAddressedActionMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t action,uint8_t val0,uint8_t val1);
    uint8_t createAddressedStatusMessage(uint8_t status_code, uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t p0,uint8_t p1,uint8_t p2);
    uint8_t createInitialRegisterMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t status,uint8_t val0,uint8_t val1,uint8_t val2);

    uint8_t createEmergencyBroadcast(uint8_t group);
    uint8_t createEmergency(uint8_t serverAddr,uint16_t nodeid);
    uint8_t createBackToNormalBroadcast(uint8_t group);
    uint8_t createBackToNormal(uint8_t serverAddr,uint16_t nodeid);
    uint8_t createRestoreDefaultConfig(uint8_t serverAddr,uint16_t nodeid,uint8_t nodeAddr);
    uint8_t createStatusMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t status);
    uint8_t createACKMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t status);
    uint8_t createServerAutoEnum(uint16_t id);   
    uint8_t createCarAutoEnum(uint16_t id);   
    uint8_t createNodeId(uint16_t id);   
    uint8_t createCarId(uint16_t id);   
    uint8_t createRCId(uint16_t id);  
	uint8_t createRCCarRegister(uint16_t carid, uint8_t serverid);   
    uint8_t createRCCarRegisterAck(uint16_t carid, uint8_t serverid); 
    uint8_t createAcquire(uint16_t carid, uint8_t serverid);   
    uint8_t createAcquireAck(uint16_t carid, uint8_t serverid);   
	uint8_t createAcquireNAck(uint16_t carid, uint8_t serverid);   
    uint8_t createCarRelease(uint16_t carid, uint8_t serverid);   
    uint8_t createCarReleaseAck(uint16_t carid, uint8_t serverid);   
    uint8_t createCarKeepAlive(uint16_t carid, uint8_t serverid);   
    uint8_t createRCKeepAlive(uint16_t carid, uint8_t serverid);   
    uint8_t createSaveParam(uint16_t carid, uint8_t serverid, uint8_t idx, uint8_t value );   
    uint8_t createRCTurn(uint16_t carid, uint8_t serverid,uint8_t angle, uint8_t direction);
    uint8_t createRCMove(uint16_t carid, uint8_t serverid,uint8_t speed, uint8_t direction);
    uint8_t createCarLightOnOff(uint16_t carid, uint8_t serverid);   
    uint8_t createCarBreakLightOnOff(uint16_t carid, uint8_t serverid);   
    uint8_t createLowBattery(uint8_t serverAddr,uint16_t nodeid);
    uint8_t createStopCar(uint16_t carid, uint8_t serverAddr);

    void dumpBuffer();
    void dumpRadioBuffer();
    void resetToDefault();    
    states convertFromInt(uint8_t s);
protected:

private:
    log4cpp::Category *logger;
    uint16_t radioID;
    uint8_t buffer[MESSAGE_SIZE]; // the message set to this class    
    uint8_t messageLength;
    uint8_t params[PARAMETERS_SIZE];
    uint16_t nodenumber;
    uint16_t origin;
    uint8_t radioBuffer[MESSAGE_SIZE];// the message created by the create methods    
    uint8_t radioMessageLength;    
    uint16_t word(uint8_t a, uint8_t b);
    uint8_t lowByte(uint16_t);
    uint8_t highByte(uint16_t);
};

#endif // __CSRD__H
