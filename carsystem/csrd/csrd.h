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
   LOWBATTERY
};

//status
typedef enum STATUS {
   ACTIVE=0,
   INACTIVE,
   CHARGING,
   PANNE,
   REGISTERED,
   NOT_REGISTERED,
   WAITING_REGISTRATION
};

//which part
typedef enum objects_enum {
  LEFT_LIGHT=0,
  RIGHT_LIGHT,
  BREAK_LIGHT,
  REAR_BREAK_LIGHT,
  SIRENE_LIGHT,
  FRONT_LIGHT,
  MOTOR,
  IR_RECEIVE,
  IR_SEND
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

    uint8_t getGroup();
    uint8_t getElement();
    uint8_t getState();
    uint8_t getStatus();
    uint8_t getParamIdx();
    uint8_t getVal0();
    uint8_t getVal1();
    uint8_t getVal2();
    uint16_t getNodeNumber();

    bool sendBroadcastOPMessage(uint8_t serverAddr,uint8_t group,uint8_t element,uint8_t state,uint8_t val0,uint8_t val1,uint8_t val2);
    bool sendBroadcastRequestRegister(uint8_t serverAddr,uint8_t group);
    bool sendBroadcastWriteMessage(uint8_t serverAddr,uint8_t group,uint8_t element,uint8_t param_idx,uint8_t val0,uint8_t val1,uint8_t val2);

    bool sendAddressedWriteMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t param_idx,uint8_t val0,uint8_t val1);
    bool sendAddressedReadMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t param_idx);
    bool sendAddressedOPMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t element,uint8_t state,uint8_t val0,uint8_t val1);

    bool sendInitialRegisterMessage(uint8_t serverAddr,uint16_t nodeid,uint8_t status,uint8_t val0,uint8_t val1,uint8_t val2);
    bool sendEmergencyBroadcast(uint8_t serverAddr,uint8_t group);
    bool sendEmergency(uint8_t serverAddr,uint16_t nodeid);
    bool sendBackToNormalBroadcast(uint8_t serverAddr,uint8_t group);
    bool sendBackToNormal(uint8_t serverAddr,uint16_t nodeid);
    
    bool sendLowBattery(uint8_t serverAddr,uint16_t nodeid);
    void resetToDefault();
    uint8_t getLength(){return length;};


    bool isRadioOn();
    uint16_t getSender(){return origin;};
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
    int radioBuffSize;
};



#endif // __CSRD__H
