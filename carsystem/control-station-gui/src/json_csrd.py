from radio_protocol import *
import json
import csrd

J_READ = "READ"
J_WRITE = "WRITE"
J_OPERATION = "OPERATION"
J_ACTION = "ACTION"
J_UNKOWN = "UNKNOW"
J_FROM = "from"
J_TO = "to"
J_NAME = "name"
J_DATE = "date"
J_ID = "id"
J_TYPE = "type"
J_ACTION_TYPE = "action_type"
J_GROUP = "group"
J_ELEMENT = "element"
J_NEXTSTATE = "next_state"
J_PARAM_INDEX = "param_index"
J_ACTION_PARAM = "action"
J_VALUES = "values"
J_STATUS_TYPE = "status_type"
J_STATUS = "status"
J_PARAMS = "params"
J_NODE_ID = "nodeid"
J_TYPE_BROADCAST = "BROADCAST"
J_TYPE_ADDRESSED = "ADDRESSED"
J_TYPE_STATUS = "STATUS"
J_TYPE_EMPTY = "EMPTY"


def json_to_csrd(message, logger):
    buffer = []
    try:
        j = json.loads(message)
    except Exception as e:
        logger.error(f"Failed to load json message: {e}")
        return None

    if not is_message_valid(j, logger):
        logger.error("Json message is invalid. [%s]", message)
        return None

    buffer[0] = createBuffer0(j)
    buffer[1] = createBuffer1(j)
    buffer[2] = createBuffer2(j)
    buffer[3] = createBuffer3(j)
    buffer[4] = createBuffer4(j)
    buffer[5] = createBuffer5(j)
    buffer[6] = createBuffer6(j)
    buffer[7] = createBuffer7(j)

    csrd_message = csrd.CSRD(logger, buffer, MESSAGE_SIZE)

    if j[J_TO][J_ID] is not None:
        to_radio = 0
    else:
        to_radio = int(j[J_TO][J_ID])

    if j[J_FROM][J_ID] is not None:
        from_radio = 0
    else:
        from_radio = int(j[J_FROM][J_ID])

    csrd_message.setTo(to_radio)
    csrd_message.setFrom(from_radio)
    return csrd_message


def exists(message, tag, logger):
    if message[tag] is not None:
        return message[tag]

    logger.error(f"Json message missing field: {tag}")
    return None


def is_message_valid(message, logger):
    # Check the presence of the obligatory message fields

    json_from = exists(message, J_FROM, logger)
    if json_from is not None:
        if exists(json_from, J_NAME, logger) is None:
            return False
        if exists(json_from, J_ID, logger) is None:
            return False
    else:
        return False

    json_to = exists(message, J_FROM, logger)
    if json_to is not None:
        if exists(json_to, J_NAME, logger) is None:
            return False
        if exists(json_to, J_ID, logger) is None:
            return False
    else:
        return False

    if exists(message, J_TYPE, logger) is None:
        return False
    else:
        # check for specific types
        # exception here is status message
        message_type = message[J_TYPE]
        if message_type == J_TYPE_STATUS:
            if exists(message, J_STATUS_TYPE, logger) is None:
                return False

            if exists(message, J_NODE_ID, logger) is None:
                return False

            if exists(message, J_STATUS, logger) is None:
                return False

        elif message_type == J_TYPE_BROADCAST:
            if exists(message, J_ACTION_TYPE, logger) is None:
                return False
            else:
                action_type = message[J_ACTION_TYPE]
                if action_type == J_OPERATION:
                    if exists(message, J_NEXTSTATE, logger) is None:
                        return False
                elif action_type == J_ACTION:
                    if exists(message, J_ACTION_PARAM, logger) is None:
                        return False

            if exists(message, J_GROUP, logger) is None:
                return False

            if exists(message, J_ELEMENT, logger) is None:
                return False

            if exists(message, J_VALUES, logger) is None:
                return False

    return True


def createBuffer0(json_message):
    message_type = json_message[J_TYPE]
    if message_type == J_TYPE_BROADCAST:
        return RP_BROADCAST

    elif message_type == J_TYPE_ADDRESSED:
        return RP_ADDRESSED

    elif message_type == J_TYPE_STATUS:
        return RP_STATUS

    else:
        return RP_UNKOWN


def createBuffer1(json_message):
    message_type = json_message[J_TYPE]
    if message_type == J_TYPE_BROADCAST or message_type == J_TYPE_ADDRESSED:
        action_type = json_message[J_ACTION_TYPE]
        if action_type == J_OPERATION:
            return RP_OPERATION

        elif action_type == J_ACTION:
            return RP_ACTION

        elif action_type == J_WRITE:
            return RP_WRITE

        elif action_type == J_READ:
            return RP_READ

        else:
            return RP_UNKOWN

    elif message_type == J_TYPE_STATUS:
        status_type = int(json_message[J_STATUS_TYPE])
        return status_type

    else:
        return RP_UNKOWN


def createBuffer2(json_message):
    message_type = json_message[J_TYPE]
    if message_type == J_TYPE_BROADCAST:
        group = int(json_message[J_GROUP])
        return group

    if message_type == J_TYPE_ADDRESSED:
        nodeid = int(json_message[J_NODE_ID])
        return highByte(nodeid)

    elif message_type == J_TYPE_STATUS:
        nodeid = int(json_message[J_NODE_ID])
        return highByte(nodeid)

    else:
        return RP_UNKOWN


def createBuffer3(json_message):
    message_type = json_message[J_TYPE]
    if message_type == J_TYPE_BROADCAST:
        group = int(json_message[J_ELEMENT])
        return group

    if message_type == J_TYPE_ADDRESSED:
        nodeid = int(json_message[J_NODE_ID])
        return lowByte(nodeid)

    elif message_type == J_TYPE_STATUS:
        nodeid = int(json_message[J_NODE_ID])
        return lowByte(nodeid)

    else:
        return RP_UNKOWN


def createBuffer4(json_message):
    message_type = json_message[J_TYPE]
    if message_type == J_TYPE_BROADCAST:
        action_type = json_message[J_ACTION_TYPE]
        if action_type == J_OPERATION:
            next_state = int(json_message[J_NEXTSTATE])
            return next_state

        elif action_type == J_ACTION:
            action = int(json_message[J_ACTION_PARAM])
            return action

        elif action_type == J_WRITE:
            param_index = int(json_message[J_PARAM_INDEX])
            return param_index

        elif action_type == J_READ:
            param_index = int(json_message[J_PARAM_INDEX])
            return param_index

        else:
            return RP_UNKOWN

    if message_type == J_TYPE_ADDRESSED:
        element = int(json_message[J_ELEMENT])
        return element

    elif message_type == J_TYPE_STATUS:
        element = int(json_message[J_ELEMENT])
        return element

    else:
        return RP_UNKOWN


def createBuffer5(json_message):
    message_type = json_message[J_TYPE]
    if message_type == J_TYPE_BROADCAST:
        values = json_message[J_VALUES]
        if values[0] is None:
            return 0
        else:
            value = int(values[0])
            return value

    if message_type == J_TYPE_ADDRESSED:
        action_type = json_message[J_ACTION_TYPE]
        if action_type == J_OPERATION:
            next_state = int(json_message[J_NEXTSTATE])
            return next_state

        elif action_type == J_ACTION:
            action = int(json_message[J_ACTION_PARAM])
            return action

        elif action_type == J_WRITE:
            param_index = int(json_message[J_PARAM_INDEX])
            return param_index

        elif action_type == J_READ:
            param_index = int(json_message[J_PARAM_INDEX])
            return param_index

        else:
            return RP_UNKOWN

    elif message_type == J_TYPE_STATUS:
        status_type = int(json_message[J_STATUS_TYPE])
        if status_type == RP_REPORT_ACK:
            element = int(json_message[J_ELEMENT])
            return element

        else:
            values = json_message[J_VALUES]
            if values[0] is None:
                return 0
            else:
                value = int(values[0])
                return value
    else:
        return RP_UNKOWN


def createBuffer6(json_message):
    message_type = json_message[J_TYPE]
    if message_type == J_TYPE_BROADCAST:
        values = json_message[J_VALUES]
        if values[1] is None:
            return 0
        else:
            value = int(values[1])
            return value

    if message_type == J_TYPE_ADDRESSED:
        values = json_message[J_VALUES]
        if values[0] is None:
            return 0
        else:
            value = int(values[0])
            return value

    elif message_type == J_TYPE_STATUS:
        status_type = int(json_message[J_STATUS_TYPE])
        index = 1
        if status_type == RP_REPORT_ACK:
            index = 0

        values = json_message[J_VALUES]
        if values[index] is None:
            return 0
        else:
            value = int(values[index])
            return value

    else:
        return RP_UNKOWN


def createBuffer7(json_message):
    message_type = json_message[J_TYPE]
    if message_type == J_TYPE_BROADCAST:
        values = json_message[J_VALUES]
        if values[2] is None:
            return 0
        else:
            value = int(values[2])
            return value

    if message_type == J_TYPE_ADDRESSED:
        values = json_message[J_VALUES]
        if values[1] is None:
            return 0
        else:
            value = int(values[1])
            return value

    elif message_type == J_TYPE_STATUS:
        status_type = int(json_message[J_STATUS_TYPE])
        index = 2
        if status_type == RP_REPORT_ACK:
            index = 1

        values = json_message[J_VALUES]
        if values[index] is None:
            return 0
        else:
            value = int(values[index])
            return value
    else:
        return RP_UNKOWN


def lowByte(a):
    b = a & 0x00ff
    return b


def highByte(a):
    b = a >> 8
    return b
