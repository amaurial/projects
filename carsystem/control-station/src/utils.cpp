#include "utils.h"

using namespace nlohmann;

string csrdToJson(CSRD *message){
    string result;
    uint8_t buf[MESSAGE_SIZE];
    json j;
    
    if (message == NULL){        
        j = json::parse(EMPTY_MESSAGE);
        return j.dump();        
    }

    if (!message->getMessageBuffer(buf)){
        j = json::parse(EMPTY_MESSAGE);
        return j.dump();
    }

    switch (buf[0])
    {
        case RP_BROADCAST:
            result = convertBroadcastMessage(buf, message->getMessageLength());            
            break;
        case RP_ADDRESSED:
            result = convertAddressedMessage(buf, message->getMessageLength());
            break;
        case RP_STATUS:
            result = convertStatusMessage(buf, message->getMessageLength());;
            break;    
        default:
            result = EMPTY_MESSAGE;                        
            break;
    }
    
    //todo make locale configurable
    // locale -a list the possible locales
    boost::algorithm::replace_all(result, "$DATE", message->getTimeAsString("en_GB.utf8"));
    boost::algorithm::replace_all(result, "$TO_ID", to_string(message->getTo()));
    boost::algorithm::replace_all(result, "$TO_NAME", to_string(message->getTo()));
    boost::algorithm::replace_all(result, "$FROM_ID", to_string(message->getFrom()));
    boost::algorithm::replace_all(result, "$FROM_NAME", to_string(message->getFrom()));
    cout << result << endl;
    j = json::parse(result);
    return j.dump();       

}

string convertBroadcastMessage(uint8_t *buf, uint8_t size){
    string result = BROADCAST_MESSAGE_TEMPLATE;    
    // common variables
    boost::algorithm::replace_all(result, "$GROUP", to_string(buf[2]));
    boost::algorithm::replace_all(result, "$ELEMENT", to_string(buf[3]));
    
    switch (buf[1])
    {
        case RP_WRITE:
            boost::algorithm::replace_all(result, "$ACTION_TYPE", J_WRITE);                        
            boost::algorithm::replace_all(result, "$NEXT_STATE", "");
            boost::algorithm::replace_all(result, "$PARAM_INDEX", to_string(buf[4]));
            boost::algorithm::replace_all(result, "$ACTION", "");
            boost::algorithm::replace_all(result, "$VAL_F", to_string(buf[5]));
            boost::algorithm::replace_all(result, "$VAL_G", to_string(buf[6]));
            boost::algorithm::replace_all(result, "$VAL_H", to_string(buf[7]));
            break;
        case RP_READ:
            boost::algorithm::replace_all(result, "$ACTION_TYPE", J_READ);
            boost::algorithm::replace_all(result, "$NEXT_STATE", "");
            boost::algorithm::replace_all(result, "$PARAM_INDEX", to_string(buf[4]));
            boost::algorithm::replace_all(result, "$ACTION", "");
            boost::algorithm::replace_all(result, "$VAL_F", to_string(buf[5]));
            boost::algorithm::replace_all(result, "$VAL_G", to_string(buf[6]));
            boost::algorithm::replace_all(result, "$VAL_H", to_string(buf[7]));
            break;
        case RP_ACTION:
            boost::algorithm::replace_all(result, "$ACTION_TYPE", J_ACTION);
            boost::algorithm::replace_all(result, "$NEXT_STATE", "");
            boost::algorithm::replace_all(result, "$PARAM_INDEX", "" );
            boost::algorithm::replace_all(result, "$ACTION", to_string(buf[4]));
            boost::algorithm::replace_all(result, "$VAL_F", to_string(buf[5]));
            boost::algorithm::replace_all(result, "$VAL_G", to_string(buf[6]));
            boost::algorithm::replace_all(result, "$VAL_H", to_string(buf[7]));
            break;
        case RP_OPERATION:
            boost::algorithm::replace_all(result, "$ACTION_TYPE", J_OPERATION);
            boost::algorithm::replace_all(result, "$NEXT_STATE", to_string(buf[4]));
            boost::algorithm::replace_all(result, "$PARAM_INDEX", "");
            boost::algorithm::replace_all(result, "$ACTION", "");
            boost::algorithm::replace_all(result, "$VAL_F", to_string(buf[5]));
            boost::algorithm::replace_all(result, "$VAL_G", to_string(buf[6]));
            boost::algorithm::replace_all(result, "$VAL_H", to_string(buf[7]));
            break;
        default:
            boost::algorithm::replace_all(result, "$ACTION_TYPE", J_UNKOWN);
            boost::algorithm::replace_all(result, "$NEXT_STATE", "");
            boost::algorithm::replace_all(result, "$PARAM_INDEX", "");
            boost::algorithm::replace_all(result, "$ACTION", "");
            boost::algorithm::replace_all(result, "$VAL_F", to_string(buf[5]));
            boost::algorithm::replace_all(result, "$VAL_G", to_string(buf[6]));
            boost::algorithm::replace_all(result, "$VAL_H", to_string(buf[7]));
            break;
    }
    return result;    
}

string convertAddressedMessage(uint8_t *buf, uint8_t size){
    string result = ADDRESSED_MESSAGE_TEMPLATE;    
    // common variables
    uint16_t nodeid = buf[2] << 8 | buf[3];
    boost::algorithm::replace_all(result, "$NODEID", to_string(nodeid));
    boost::algorithm::replace_all(result, "$ELEMENT", to_string(buf[4]));
    
    switch (buf[1])
    {
        case RP_WRITE:
            boost::algorithm::replace_all(result, "$ACTION_TYPE", J_WRITE);                        
            boost::algorithm::replace_all(result, "$NEXT_STATE", "");
            boost::algorithm::replace_all(result, "$PARAM_INDEX", to_string(buf[5]));
            boost::algorithm::replace_all(result, "$ACTION", "");            
            boost::algorithm::replace_all(result, "$VAL_G", to_string(buf[6]));
            boost::algorithm::replace_all(result, "$VAL_H", to_string(buf[7]));
            break;
        case RP_READ:
            boost::algorithm::replace_all(result, "$ACTION_TYPE", J_READ);
            boost::algorithm::replace_all(result, "$NEXT_STATE", "");
            boost::algorithm::replace_all(result, "$PARAM_INDEX", to_string(buf[5]));
            boost::algorithm::replace_all(result, "$ACTION", "");            
            boost::algorithm::replace_all(result, "$VAL_G", to_string(buf[6]));
            boost::algorithm::replace_all(result, "$VAL_H", to_string(buf[7]));
            break;
        case RP_ACTION:
            boost::algorithm::replace_all(result, "$ACTION_TYPE", J_ACTION);
            boost::algorithm::replace_all(result, "$NEXT_STATE", "");
            boost::algorithm::replace_all(result, "$PARAM_INDEX", "");
            boost::algorithm::replace_all(result, "$ACTION", to_string(buf[5]));            
            boost::algorithm::replace_all(result, "$VAL_G", to_string(buf[6]));
            boost::algorithm::replace_all(result, "$VAL_H", to_string(buf[7]));
            break;
        case RP_OPERATION:
            boost::algorithm::replace_all(result, "$ACTION_TYPE", J_OPERATION);
            boost::algorithm::replace_all(result, "$NEXT_STATE", to_string(buf[5]));
            boost::algorithm::replace_all(result, "$PARAM_INDEX", "");
            boost::algorithm::replace_all(result, "$ACTION", "");            
            boost::algorithm::replace_all(result, "$VAL_G", to_string(buf[6]));
            boost::algorithm::replace_all(result, "$VAL_H", to_string(buf[7]));
            break;
        default:
            boost::algorithm::replace_all(result, "$ACTION_TYPE", J_UNKOWN);
            boost::algorithm::replace_all(result, "$NEXT_STATE", "");
            boost::algorithm::replace_all(result, "$PARAM_INDEX", to_string(buf[5]));
            boost::algorithm::replace_all(result, "$ACTION", "");            
            boost::algorithm::replace_all(result, "$VAL_G", to_string(buf[6]));
            boost::algorithm::replace_all(result, "$VAL_H", to_string(buf[7]));
            break;
    }
    return result;    
}

string convertStatusMessage(uint8_t *buf, uint8_t size){
    string result = STATUS_MESSAGE_TEMPLATE;    
    // common variables
    uint16_t nodeid = buf[2] << 8 | buf[3];
    boost::algorithm::replace_all(result, "$NODEID", to_string(nodeid));
    boost::algorithm::replace_all(result, "$STATUS_TYPE", getStatusType(buf[1]));    
    
    switch (buf[1])
    {
        case RP_REPORT_STATUS:
            boost::algorithm::replace_all(result, "$ELEMENT", "");                                    
            boost::algorithm::replace_all(result, "$STATUS", to_string(buf[4]));                       
            boost::algorithm::replace_all(result, "$VAL_F", to_string(buf[5]));
            boost::algorithm::replace_all(result, "$VAL_G", to_string(buf[6]));
            boost::algorithm::replace_all(result, "$VAL_H", to_string(buf[7]));
            break;
        case RP_REPORT_ACK:
            boost::algorithm::replace_all(result, "$ELEMENT", to_string(buf[4]));
            boost::algorithm::replace_all(result, "$STATUS", to_string(buf[5]));
            boost::algorithm::replace_all(result, "$VAL_G", to_string(buf[6]));
            boost::algorithm::replace_all(result, "$VAL_H", to_string(buf[7]));
            break;
        case RP_INITIALREG:
            boost::algorithm::replace_all(result, "$ELEMENT", "");                                    
            boost::algorithm::replace_all(result, "$STATUS", to_string(buf[4]));
            boost::algorithm::replace_all(result, "$VAL_F", to_string(buf[5]));
            boost::algorithm::replace_all(result, "$VAL_G", to_string(buf[6]));
            boost::algorithm::replace_all(result, "$VAL_H", to_string(buf[7]));
            break;        
        default:
            boost::algorithm::replace_all(result, "$ELEMENT", "");                                    
            boost::algorithm::replace_all(result, "$STATUS", J_UNKOWN);
            boost::algorithm::replace_all(result, "$VAL_F", to_string(buf[5]));
            boost::algorithm::replace_all(result, "$VAL_G", to_string(buf[6]));
            boost::algorithm::replace_all(result, "$VAL_H", to_string(buf[7]));
            break;
    }
    return result;    
}

string getStatusType(uint8_t status){
    switch (status)
    {
        case RP_REPORT_STATUS:
            return "REPORT";
            break;
        case RP_INITIALREG:
            return "REGISTRATION";
            break;
        case RP_REPORT_ACK:
            return "ACK";
            break;
        default:
            return J_UNKOWN;
            break;
    }
    return J_UNKOWN;
}