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
            boost::algorithm::replace_all(result, "$ELEMENT", to_string(buf[5]));
            boost::algorithm::replace_all(result, "$STATUS", to_string(buf[4]));
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

bool jsonToCSRD(CSRD *message, string jsonMessage, log4cpp::Category *logger){
    uint8_t buffer[MESSAGE_SIZE];
    
    // try to parse
    json j;
    try{        
        j = json::parse(jsonMessage);
    }
    catch(const exception &ex){
        logger->error("Failed to parse json message [%s] reason:%s", jsonMessage.c_str(), ex.what());  
        return false;      
    }
    
    if (!isMessageValid(j)){
        logger->error("Json message is invalid. [%s]", jsonMessage)
        return false;
    }    
    
    buffer[0] = createBuffer0(j);
    buffer[1] = createBuffer1(j);
    buffer[2] = createBuffer2(j);
    buffer[3] = createBuffer3(j);
    buffer[4] = createBuffer4(j);
    buffer[5] = createBuffer5(j);
    buffer[6] = createBuffer6(j);
    buffer[7] = createBuffer7(j);

    message->setMessage(0, buffer, MESSAGE_SIZE);
    return true;
}

bool isMessageValid(json jsonMessage, log4cpp::Category *logger){

    // Check the presence of the obligatory message fields

    json json_from = exists(jsonMessage, J_FROM, logger);
    if (json_from != nullptr){
        if (exists(json_from, J_NAME, logger) == nullptr){            
            return false;
        }

        if (exists(json_from, J_ID, logger) == nullptr){            
            return false;
        }
    }
    else{        
        return false;
    }    

    json json_to = exists(jsonMessage, J_FROM,logger);
    if (json_to != nullptr){
        if (exists(json_to, J_NAME, logger) == nullptr){            
            return false;
        }

        if (exists(json_to, J_ID, logger) == nullptr){            
            return false;
        }
    }
    else{        
        return false;
    }    src/utils.h

    if (exists(jsonMessage, J_TYPE, logger) == nullptr){        
        return false;
    }
    else{
        // check for specific types
        // exception here is status message
        string message_type = jsonMessage[J_TYPE].as<string>();;
        if (message_type == J_TYPE_STATUS){
            
            if (exists(jsonMessage, J_STATUS_TYPE, logger) == nullptr){            
                return false;
            }

            if (exists(jsonMessage, J_NODE_ID, logger) == nullptr){                
                return false;
            }

            if (exists(jsonMessage, J_STATUS, logger) == nullptr){                
                return false;
            }
        }
        else if (message_type == J_TYPE_BROADCAST){
            
            if (exists(jsonMessage, J_ACTION_TYPE, logger) == nullptr){                
                return false;
            }
            else{
                string action_type = jsonMessage[J_ACTION_TYPE].as<string>();;
                if (action_type == J_OPERATION){
                    if (exists(jsonMessage, J_NEXTSTATE, logger) == nullptr){                        
                        return false;
                    }
                }
                else if (action_type == J_ACTION){
                    if (exists(jsonMessage, J_ACTION, logger) == nullptr){                        
                        return false;
                    }
                }
            }

            if (exists(jsonMessage, J_GROUP, logger) == nullptr){                
                return false;
            }

            if (exists(jsonMessage, J_ELEMENT, logger) == nullptr){                
                return false;
            }

            if (exists(jsonMessage, J_VALUES, logger) == nullptr){                
                return false;
            }
        }
    }

    return true;
}

json exists(json jsonMessage, string tag, log4cpp::Category *logger){
    if (jsonMessage.find(tag)){
        return jsonMessage[tag];
    }
    logger->error("Json message missing field: %s", tag;
    return nullptr;
}

uint8_t createBuffer0(json jsonMessage){
    string message_type = jsonMessage[J_TYPE].as<string>();;
    if (message_type == J_TYPE_BROADCAST){
        return RP_BROADCAST;
    }
    else if (message_type == J_TYPE_ADDRESSED){
        return RP_ADDRESSED;
    }
    else if (message_type == J_TYPE_STATUS){
        return RP_STATUS;
    }
    else{
        return RP_UNKOWN;
    }
}

uint8_t createBuffer1(json jsonMessage){
    string message_type = jsonMessage[J_TYPE].as<string>();;
    if (message_type == J_TYPE_BROADCAST || message_type == J_TYPE_ADDRESSED){
        string action_type = jsonMessage[J_ACTION_TYPE];
        if (action_type == J_OPERATION){
            return RP_OPERATION;
        }
        else if (action_type == J_ACTION){
            return RP_ACTION;
        }
        else if (action_type == J_WRITE){
            return RP_WRITE;
        }
        else if (action_type == J_READ){
            return RP_READ;
        }
        else{
            return RP_UNKOWN;
        }
    }    
    else if (message_type == J_TYPE_STATUS){
        uint8_t status_type = (uint8_t) jsonMessage[J_STATUS_TYPE].as<int>();        
        return status_type;
    }
    else{
        return RP_UNKOWN;
    }
}

uint8_t createBuffer2(json jsonMessage){
    string message_type = jsonMessage[J_TYPE].as<string>();;
    if (message_type == J_TYPE_BROADCAST){
        uint8_t group = (uint8_t) jsonMessage[J_GROUP].as<int>();        
        return group;
    }
    if (message_type == J_TYPE_ADDRESSED){
        uint16_t nodeid = (uint16_t) jsonMessage[J_NODE_ID].as<int>();        
        return highByte(nodeid);
    }
    else if (message_type == J_TYPE_STATUS){
        uint16_t nodeid = (uint16_t) jsonMessage[J_NODE_ID].as<int>();        
        return highByte(nodeid);
    }
    else{
        return RP_UNKOWN;
    }
}

uint8_t createBuffer3(json jsonMessage){
    string message_type = jsonMessage[J_TYPE].as<string>();;
    if (message_type == J_TYPE_BROADCAST){
        uint8_t group = (uint8_t) jsonMessage[J_ELEMENT].as<int>();        
        return group;
    }
    if (message_type == J_TYPE_ADDRESSED){
        uint16_t nodeid = (uint16_t) jsonMessage[J_NODE_ID].as<int>();        
        return lowByte(nodeid);
    }
    else if (message_type == J_TYPE_STATUS){
        uint16_t nodeid = (uint16_t) jsonMessage[J_NODE_ID].as<int>();        
        return highlowByteByte(nodeid);
    }
    else{
        return RP_UNKOWN;
    }
}

uint8_t createBuffer4(json jsonMessage){
    string message_type = jsonMessage[J_TYPE].as<string>();
    if (message_type == J_TYPE_BROADCAST){
        string action_type = jsonMessage[J_ACTION_TYPE].as<string>();;
        if (action_type == J_OPERATION){
            uint8_t next_state = (uint8_t) jsonMessage[J_NEXTSTATE].as<int>();        
            return next_state;
        }
        else if (action_type == J_ACTION){
            uint8_t action = (uint8_t) jsonMessage[J_ACTION].as<int>();        
            return action;
        }
        else if (action_type == J_WRITE){
            uint8_t param_index = (uint8_t) jsonMessage[J_PARAM_INDEX].as<int>();        
            return param_index;
        }
        else if (action_type == J_READ){
            uint8_t param_index = (uint8_t) jsonMessage[J_PARAM_INDEX].as<int>();        
            return param_index;
        }
        else{
            return RP_UNKOWN;
        }
    }
    if (message_type == J_TYPE_ADDRESSED){
        uint8_t element = (uint8_t) jsonMessage[J_ELEMENT].as<int>();        
        return element;
    }
    else if (message_type == J_TYPE_STATUS){        
        uint8_t element = (uint8_t) jsonMessage[J_ELEMENT].as<int>();
        return element;
    }
    else{
        return RP_UNKOWN;
    }
}

uint8_t createBuffer5(json jsonMessage){
    string message_type = jsonMessage[J_TYPE].as<string>();;
    if (message_type == J_TYPE_BROADCAST){
        json values = jsonMessage[J_VALUES];
        if (values[0].is_null()){
            return 0;
        }
        else{
            uint8_t value = (uint8_t)value[0].as<int>();
            return value;
        }
    }
    if (message_type == J_TYPE_ADDRESSED){
        string action_type = jsonMessage[J_ACTION_TYPE].as<string>();;
        if (action_type == J_OPERATION){
            uint8_t next_state = (uint8_t) jsonMessage[J_NEXTSTATE].as<int>();        
            return next_state;
        }
        else if (action_type == J_ACTION){
            uint8_t action = (uint8_t) jsonMessage[J_ACTION].as<int>();        
            return action;
        }
        else if (action_type == J_WRITE){
            uint8_t param_index = (uint8_t) jsonMessage[J_PARAM_INDEX].as<int>();        
            return param_index;
        }
        else if (action_type == J_READ){
            uint8_t param_index = (uint8_t) jsonMessage[J_PARAM_INDEX].as<int>();        
            return param_index;
        }
        else{
            return RP_UNKOWN;
        }
    }
    else if (message_type == J_TYPE_STATUS){
        uint8_t status_type = (uint8_t) jsonMessage[J_STATUS_TYPE].as<int>();        
        if (status_type == RP_REPORT_ACK){
            uint8_t element = (uint8_t) jsonMessage[J_ELEMENT].as<int>();        
            return element;
        }
        else{
            json values = jsonMessage[J_VALUES];
            if (values[0].is_null()){
                return 0;
            }
            else{
                uint8_t value = (uint8_t)value[0].as<int>();
                return value;
            }
        }
    }
    else{
        return RP_UNKOWN;
    }
}

uint8_t createBuffer6(json jsonMessage){
    string message_type = jsonMessage[J_TYPE].as<string>();;
    if (message_type == J_TYPE_BROADCAST){
        json values = jsonMessage[J_VALUES];
        if (values[1].is_null()){
            return 0;
        }
        else{
            uint8_t value = (uint8_t)value[1].as<int>();
            return value;
        }
    }
    if (message_type == J_TYPE_ADDRESSED){
        json values = jsonMessage[J_VALUES];
        if (values[0].is_null()){
            return 0;
        }
        else{
            uint8_t value = (uint8_t)value[0].as<int>();
            return value;
        }
    }
    else if (message_type == J_TYPE_STATUS){
        uint8_t status_type = (uint8_t) jsonMessage[J_STATUS_TYPE].as<int>();
        uint8_t index = 1;        
        if (status_type == RP_REPORT_ACK){
            index = 0;
        }
        
        json values = jsonMessage[J_VALUES];
        if (values[index].is_null()){
            return 0;
        }
        else{
            uint8_t value = (uint8_t)value[index].as<int>();
            return value;
        }        
    }
    else{
        return RP_UNKOWN;
    }
}

uint8_t createBuffer7(json jsonMessage){
    string message_type = jsonMessage[J_TYPE].as<string>();;
    if (message_type == J_TYPE_BROADCAST){
        json values = jsonMessage[J_VALUES];
        if (values[2].is_null()){
            return 0;
        }
        else{
            uint8_t value = (uint8_t)value[2].as<int>();
            return value;
        }
    }
    if (message_type == J_TYPE_ADDRESSED){
        json values = jsonMessage[J_VALUES];
        if (values[1].is_null()){
            return 0;
        }
        else{
            uint8_t value = (uint8_t)value[1].as<int>();
            return value;
        }
    }
    else if (message_type == J_TYPE_STATUS){
        uint8_t status_type = (uint8_t) jsonMessage[J_STATUS_TYPE].as<int>();
        uint8_t index = 2;        
        if (status_type == RP_REPORT_ACK){
            index = 1;
        }
        
        json values = jsonMessage[J_VALUES];
        if (values[index].is_null()){
            return 0;
        }
        else{
            uint8_t value = (uint8_t)value[index].as<int>();
            return value;
        }        
    }
    else{
        return RP_UNKOWN;
    }
}