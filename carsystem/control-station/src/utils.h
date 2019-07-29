#ifndef __CSRD_UTILS_H
#define __CSRD_UTILS_H

#include <string>
#include "csrd.h"
#include "nlohmann/json.hpp"
#include "boost/algorithm/string.hpp"

#define J_READ      "READ"
#define J_WRITE     "WRITE"
#define J_OPERATION "OPERATION"
#define J_ACTION    "ACTION"
#define J_UNKOWN    "UNKNOW"

using namespace std;

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
            \"nextstate\": \"$NEXT_STATE\",\
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
            \"nextstate\": \"$NEXT_STATE\",\
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

#endif