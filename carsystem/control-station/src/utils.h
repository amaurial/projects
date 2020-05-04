#ifndef __CSRD_UTILS_H
#define __CSRD_UTILS_H

#include <string>
#include <stdint.h>
#include "csrd.h"
#include "nlohmann/json.hpp"
#include "boost/algorithm/string.hpp"

#define J_READ          "READ"
#define J_WRITE         "WRITE"
#define J_OPERATION     "OPERATION"
#define J_ACTION        "ACTION"
#define J_UNKOWN        "UNKNOW"
#define J_FROM          "from"
#define J_TO            "to"
#define J_NAME          "name"
#define J_DATE          "date"
#define J_ID            "id"
#define J_TYPE          "type"
#define J_ACTION_TYPE   "action_type"
#define J_GROUP         "group"
#define J_ELEMENT       "element"
#define J_NEXTSTATE     "next_state"
#define J_PARAM_INDEX   "param_index"
#define J_ACTION_PARAM  "action"
#define J_VALUES        "values"
#define J_STATUS_TYPE   "status_type"
#define J_STATUS        "status"
#define J_PARAMS        "params"
#define J_NODE_ID       "nodeid"
#define J_TYPE_BROADCAST "BROADCAST"
#define J_TYPE_ADDRESSED "ADDRESSED"
#define J_TYPE_STATUS    "STATUS"
#define J_TYPE_EMPTY     "EMPTY"

using namespace std;
using namespace nlohmann;

const string BROADCAST_MESSAGE_TEMPLATE = "{\
    \"date\": \"$DATE\",\
    \"csrd\": {\
            \"from\":{\
                \"name\": \"$FROM_NAME\",\
                \"id\": \"$FROM_ID\"\
            },\
            \"to\":{\
                \"name\": \"$TO_NAME\",\
                \"id\": \"$TO_ID\"\
            },\
            \"type\" : \"BROADCAST\",\
            \"action_type\" : \"$ACTION_TYPE\",\
            \"group\" : \"$GROUP\",\
            \"element\": \"$ELEMENT\",\
            \"next_state\": \"$NEXT_STATE\",\
            \"param_index\": \"$PARAM_INDEX\",\
            \"action\": \"$ACTION\",\
            \"values\": [$VAL_F , $VAL_G , $VAL_H]\
    }\
}";

const string ADDRESSED_MESSAGE_TEMPLATE = "{\
    \"date\": \"$DATE\",\
    \"csrd\":{\
            \"from\":{\
                \"name\": \"$FROM_NAME\",\
                \"id\": \"$FROM_ID\"\
            },\
            \"to\":{\
                {\"name\": \"$TO_NAME\",\
                \"id\": \"$TO_ID\"\
            },\
            \"type\" : \"ADDRESSED\",\
            \"action_type\" : \"$ACTION_TYPE\",\
            \"nodeid\" : \"$NODEID\",\
            \"element\": \"$ELEMENT\",\
            \"param_index\" : \"$PARAM_INDEX\",\
            \"next_state\": \"$NEXT_STATE\",\
            \"action\": \"$ACTION\",\
            \"values\": [$VAL_G , $VAL_H ]\
    }\
}";

const string STATUS_MESSAGE_TEMPLATE = "{\
    \"date\": \"$DATE\",\
    \"csrd\":{\
            \"from\":{\
                \"name\": \"$FROM_NAME\",\
                \"id\": \"$FROM_ID\"\
            },\
            \"to\":{\
                \"name\": \"$TO_NAME\",\
                \"id\": \"$TO_ID\"\
            },\
            \"type\" : \"STATUS\",\
            \"status_type\" : \"$STATUS_TYPE\",\
            \"nodeid\" : \"$NODEID\",\
            \"element\": \"$ELEMENT\",\
            \"status\" : \"$STATUS\",\
            \"params\" : [ $VAL_F, $VAL_G , $VAL_H ]\
    }\
}";

const string EMPTY_MESSAGE = "{\
    \"date\": \"\",\
    \"csrd\":{\
            \"from\":{\
                \"name\": \"\",\
                \"id\": \"\"\
            },\
            \"to\":{\
                \"name\": \"\",\
                \"id\": \"\"\
            },\
            \"type\" : \"EMPTY\",\
            \"status_type\" : \"\",\
            \"nodeid\" : \"\",\
            \"element\": \"\",\
            \"status\" : \"\",\
            \"params\" : []\
    }\
}";


string convertBroadcastMessage(uint8_t *buf, uint8_t size);
string convertAddressedMessage(uint8_t *buf, uint8_t size);
string convertStatusMessage(uint8_t *buf, uint8_t size);
string getStatusType(uint8_t status);
string csrdToJson(CSRD *message);
bool jsonToCSRD(CSRD *message, string jsonMessage, log4cpp::Category *logger);
bool hexaToCSRD(CSRD *message, string hexaMessage, log4cpp::Category *logger);
bool isHexaFormat(string message);
int toInteger(string s);
bool isMessageValid(json jsonMessage, log4cpp::Category *logger);
json exists(json jsonMessage, string tag, log4cpp::Category *logger);
uint8_t createBuffer0(json jsonMessage);
uint8_t createBuffer1(json jsonMessage);
uint8_t createBuffer2(json jsonMessage);
uint8_t createBuffer3(json jsonMessage);
uint8_t createBuffer4(json jsonMessage);
uint8_t createBuffer5(json jsonMessage);
uint8_t createBuffer6(json jsonMessage);
uint8_t createBuffer7(json jsonMessage);

uint8_t lowByte(uint16_t a);
uint8_t highByte(uint16_t a);

#endif