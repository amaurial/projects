#ifndef __CSRD__H
#define __CSRD__H

#include "arduino.h"
#include <SPI.h>
#include <RH_RF69.h>
#include "radio_protocol.h"
#define MESSAGE_SIZE 8

// Singleton instance of the radio driver
RH_RF69 rf69;

class CSRD {

public:
    CSR();
    init();

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


protected:

private:
    char buffer[MESSAGE_SIZE];
}



#endif // __CSRD__H
