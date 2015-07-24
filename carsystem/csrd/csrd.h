#ifndef __CSRD__H
#define __CSRD__H

#include "Arduino.h"
#include <RH_RF69.h>
#include <RHReliableDatagram.h>
#include "radio_protocol.h"
#define MESSAGE_SIZE 8

#define CSRD_DEBUG


class CSRD {

public:
    CSRD();
    void init(RH_RF69 *driver,RHReliableDatagram *manager);

    void sendMessage(char *buffer,uint16_t len);
    uint16_t getMessage(char *buffer);
    bool readMessage();

    bool isBroadcast();
    uint8_t getGroup();

    bool isAddressed();
    uint16_t getAddress();

    bool isOperation();
    uint8_t getAction();
    uint8_t getValue();

    bool isRead();
    uint8_t getReadParam();

    bool isWrite();
    uint8_t getWriteParam();
    uint8_t getWriteValue();


    void resetToDefault();
    uint8_t getLength(){return length;};

    uint16_t getNodeNumber(){return nodenumber;};

    bool isRadioOn();
    uint16_t getSender(){return origin;};
protected:

private:
    char buffer[MESSAGE_SIZE];
    RH_RF69 *driver;
    RHReliableDatagram *manager;
    uint8_t params[PARAMETERS_SIZE];
    uint16_t nodenumber;
    uint16_t origin;
    uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];//radio buffer
    uint8_t length;
    int radioBuffSize;

    void saveDefaultToMemory();
    bool saveParam(uint8_t param,uint8_t value);

};



#endif // __CSRD__H
