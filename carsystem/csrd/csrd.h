#ifndef __CSRD__H
#define __CSRD__H

#include "Arduino.h"
#include <RH_RF69.h>
#include <RHReliableDatagram.h>
#include "radio_protocol.h"
#define MESSAGE_SIZE 8

//#define CSRD_DEBUG 1

//states
typedef enum states {
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

typedef enum ACTIONS{
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
typedef enum STATUS {
   ACTIVE=0,
   INACTIVE,
   CHARGING,
   PANNE,
   REGISTERED,
   NOT_REGISTERED,
   WAITING_REGISTRATION
};

//status type
typedef enum STATUS_TYPE {
   STT_QUERY_STATUS=1,
   STT_ANSWER_STATUS,
   STT_QUERY_VALUE,
   STT_ANSWER_VALUE,
   STT_QUERY_VALUE_FAIL
};

//which part
typedef enum objects_enum {
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
    CSRD();
    bool init(RH_RF69 *driver,RHReliableDatagram *manager);

    bool sendMessage(uint8_t *sbuffer,uint8_t len,uint8_t serverAddr);
    uint8_t getMessage(uint8_t *mbuffer);
    uint8_t getMessageBuffer(uint8_t *mbuffer);
    bool readMessage();

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
    bool isCarRelease();

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
    uint8_t getId();
    uint8_t getServerId();

    bool sendBroadcastOPMessage(uint8_t group,uint8_t element,uint8_t state,uint8_t val0,uint8_t val1,uint8_t val2);
    bool sendBroadcastRequestRegister(uint8_t group);
    bool sendBroadcastWriteMessage(uint8_t group,uint8_t element,uint8_t param_idx,uint8_t val0,uint8_t val1,uint8_t val2);
    bool sendBroadcastActionMessage(uint8_t group,uint8_t element,uint8_t action,uint8_t val0,uint8_t val1,uint8_t val2);

    bool sendAddressedWriteMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t param_idx,uint8_t val0,uint8_t val1);
    bool sendAddressedReadMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t param_idx);
    bool sendAddressedOPMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t state,uint8_t val0,uint8_t val1);
    bool sendAddressedActionMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t action,uint8_t val0,uint8_t val1);
    bool sendAddressedStatusMessage(uint8_t status_code, uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t p0,uint8_t p1,uint8_t p2);
    bool sendInitialRegisterMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t status,uint8_t val0,uint8_t val1,uint8_t val2);

    bool sendEmergencyBroadcast(uint8_t group);
    bool sendEmergency(uint8_t serverAddr,uint16_t nodeid);
    bool sendBackToNormalBroadcast(uint8_t group);
    bool sendBackToNormal(uint8_t serverAddr,uint16_t nodeid);
    bool sendRestoreDefaultConfig(uint8_t serverAddr,uint16_t nodeid,uint8_t nodeAddr);
    bool sendStatusMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t status);
    bool sendACKMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t status);
    bool sendServerAutoEnum(uint8_t id);   
    bool sendCarAutoEnum(uint8_t id);   
    bool sendId(uint8_t id);   
    bool sendCarId(uint8_t id);   
    bool sendRCId(uint8_t id);   
    bool sendAcquire(uint8_t carid, uint8_t serverid);   

    bool sendLowBattery(uint8_t serverAddr,uint16_t nodeid);
    void resetToDefault();
    uint8_t getLength(){return length;};

    bool isRadioOn();

    states convertFromInt(uint8_t s);
protected:

private:
    uint8_t buffer[MESSAGE_SIZE];
    RH_RF69 *driver;
    RHReliableDatagram *manager;
    uint8_t params[PARAMETERS_SIZE];
    uint16_t nodenumber;
    uint16_t origin;
    uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];//radio buffer
    uint8_t length;
    uint8_t radioBuffSize;
    void dumpBuffer(uint8_t *pbuf);
};



#endif // __CSRD__H
