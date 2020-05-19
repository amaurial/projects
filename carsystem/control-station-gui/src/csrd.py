from enum import IntEnum
import datetime
from radio_protocol import *

class STATES(IntEnum):
    OFF = 0
    ON = 1
    STOPPING = 2
    ACCELERATING = 3
    BLINKING = 4
    EMERGENCY = 5
    NORMAL = 6
    NIGHT = 7
    DAY = 8
    ALL_BLINKING = 9


class ACTIONS(IntEnum):
    AC_LOWBATTERY = 0
    AC_RESTORE_DEFAULT_PARAMS = 1
    AC_SET_PARAM = 2
    AC_SET_SPEED = 3
    AC_ACQUIRE = 4
    AC_RELEASE = 5
    AC_ACK = 6
    AC_TURN = 7  # val0 is the angle
    AC_MOVE = 8  # val0 is the direction, val1 is the speed
    AC_FAIL = 9


class STATUS(IntEnum):
    ACTIVE = 0
    INACTIVE = 1
    CHARGING = 2
    PANNE = 3
    REGISTERED = 4
    NOT_REGISTERED = 5
    WAITING_REGISTRATION = 6


class STATUS_TYPE(IntEnum):
    STT_QUERY_STATUS = 1
    STT_ANSWER_STATUS = 2
    STT_QUERY_VALUE = 3
    STT_ANSWER_VALUE = 4
    STT_QUERY_VALUE_FAIL = 5


class CARPARTS(IntEnum):
    LEFT_LIGHT = 0
    RIGHT_LIGHT = 1
    BREAK_LIGHT = 2
    REED = 3
    SIRENE_LIGHT = 4
    FRONT_LIGHT = 5
    MOTOR = 6
    IR_RECEIVE = 7
    IR_SEND = 8
    BOARD = 9
    ALL = 255


class CSRD:

    """
    The buffer here is made of integers (int8 or unsigned char)cd pro
    """
    def __init__(self,
                 logger,
                 radioID = 0,
                 mbuffer=[RP_UNKOWN, RP_UNKOWN, RP_UNKOWN, RP_UNKOWN, RP_UNKOWN, RP_UNKOWN, RP_UNKOWN, RP_UNKOWN],
                 mbuffer_size=8):
        self.logger = logger
        self.radioID = radioID
        self.buffer = [RP_UNKOWN, RP_UNKOWN, RP_UNKOWN, RP_UNKOWN, RP_UNKOWN, RP_UNKOWN, RP_UNKOWN, RP_UNKOWN]
        self.messageLength = 0
        self.params = []
        self.nodenumber = 0
        self.origin = 0
        self.rssi = 0  # signal level
        self.id = 0
        self.to_address = 0
        self.from_address = 0
        self.flags = 0
        self.time_received = 0
        self.setMessage(radioID, mbuffer, mbuffer_size)
        self.elements = {0: "LEFT LIGHT",
                         1: "RIGHT LIGHT",
                         2: "BREAK LIGHT",
                         3: "REED",
                         4: "SIRENE",
                         5: "FRONT LIGHT",
                         6: "MOTOR",
                         7: "IR RECEIVE",
                         8: "IR SEND",
                         9: "BOARD",
                         255: "ALL"}
        self.states = {0: "OFF",
                       1: "ON",
                       2: "STOPPING",
                       3: "ACCELERATING",
                       4: "BLINKING",
                       5: "EMERGENCY",
                       6: "NORMAL",
                       7: "NIGHT",
                       8: "DAY",
                       9: "ALL_BLINKING",
                       254: "UNKOWN"}

    def resetBuffer(self):
        if self.messageLength == 0:
            return
        for i in range(0, MESSAGE_SIZE):
            self.buffer[i] = 0

    def setMessage(self, radioID, mbuffer, mbuffer_size):
        s = MESSAGE_SIZE
        if mbuffer_size < MESSAGE_SIZE:
            s = mbuffer_size

        self.resetBuffer()
        for i in range(0, mbuffer_size):
            self.buffer[i] = mbuffer[i]

        self.messageLength = s
        self.radioID = radioID
        self.time_received = datetime.datetime.now()
        return s

    def setMessageFromHexaString(self, radioID, hexaString):
        # the message is supposed to be in the format
        # 010003E700000000 - a pair for each value, totalizing 8 bytes

        if len(hexaString) < 16:
            return 0

        tempbuffer = [0, 0, 0, 0, 0, 0, 0, 0]
        pos = 0
        for i in range(0, 8):
            s = hexaString[pos:(pos+2)]
            try:
                v = int(s, 16)
            except Exception as e:
                v = -1

            if v < 0 or v > 255:
                return 0
            else:
                tempbuffer[i] = v
            pos += 2
        return self.setMessage(radioID, tempbuffer, MESSAGE_SIZE)

    def getMessageBuffer(self, mbuffer):
        for i in range(0, self.messageLength):
            mbuffer[i] = self.buffer[i]
        return self.messageLength

    def getMessageLength(self):
        return self.messageLength

    def getRadioID(self):
        return self.radioID

    def getTime(self):
        return self.time_received

    def getTimeAsString(self):
        return str(self.time_received)

    def setTo(self, to):
        self.to_address = to

    def getTo(self):
        return self.to_address

    def setFrom(self, from_address):
        self.from_address = from_address

    def getFrom(self):
        return self.from_address

    def setRssi(self, rssi):
        self.rssi = rssi

    def getRssi(self):
        return self.rssi

    def setFlags(self, flags):
        self.flags = flags

    def getFlags(self):
        return self.flags

    def isBroadcast(self):
        return self.buffer[0] == RP_BROADCAST

    def isAddressed(self):
        return self.buffer[0] == RP_ADDRESSED

    def isStatus(self):
        return self.buffer[0] == RP_STATUS

    def isQueryState(self):
        return self.isStatus() and self.buffer[1] == RP_STATUS_QUERY_STATE

    def isQueryAllStates(self):
        return self.isStatus() and self.buffer[1] == RP_STATUS_QUERY_ALL_STATES

    def isAnswerState(self):
        return self.isStatus() and self.buffer[1] == RP_STATUS_ANSWER_STATE

    def isOperation(self):
        return self.buffer[1] == RP_OPERATION

    def isAction(self):
        return self.buffer[1] == RP_ACTION

    def isRead(self):
        return self.isBroadcast() and self.buffer[1] == RP_READ

    def isWrite(self):
        return self.buffer[1] == RP_WRITE

    def isBroadcastRegister(self):
        return self.isBroadcast() and self.isAction() and self.buffer[4] == RP_AC_REGISTER

    def isInitialRegister(self):
        return self.isStatus() and self.buffer[1] == RP_STATUS_INITIAL_REGISTER

    def isMyGroup(self, mygroup):
        return self.getGroup() == 0 or self.getGroup() == mygroup

    def isLowBattery(self, serverAddr):
        # TODO review this AC_LOWBATTERY as action. It should be state
        return self.getVal0() == serverAddr and self.getState() == ACTIONS.AC_LOWBATTERY

    def isRestoreDefaultConfig(self, myid):
        return self.isAddressed() and self.getNodeNumber() == myid and self.getAction() == ACTIONS.AC_RESTORE_DEFAULT_PARAMS

    def isServerAutoEnum(self):
        return self.buffer[0] == BR_SERVER_AUTO_ENUM

    def isCarAutoEnum(self):
        return self.buffer[0] == BR_CAR_AUTO_ENUM

    def isResolutionId(self):
        return self.buffer[0] == RP_ID_RESOLUTION

    def isRCId(self):
        return self.buffer[0] == RC_ID

    def isCarId(self):
        return self.buffer[0] == CAR_ID

    def isAcquire(self):
        return self.buffer[0] == CAR_ACQUIRE

    def isAcquireAck(self):
        return self.buffer[0] == CAR_ACQUIRE_ACK

    def isRCCarRegister(self):
        return self.buffer[0] == RC_CAR_REGISTER

    def isRCCarRegisterAck(self):
        return self.buffer[0] == RC_CAR_REGISTER_ACK

    def isAcquireNAck(self):
        return self.buffer[0] == CAR_ACQUIRE_NACK

    def isCarRelease(self):
        return self.buffer[0] == CAR_RELEASE

    def isCarReleaseAck(self):
        return self.buffer[0] == CAR_RELEASE_ACK

    def isCarKeepAlive(self):
        return self.buffer[0] == CAR_KEEP_ALIVE

    def isRCKeepAlive(self):
        return self.buffer[0] == RC_KEEP_ALIVE

    def isSaveParam(self):
        return self.buffer[0] == SAVE_PARAM

    def isRCLights(self):
        return self.buffer[0] == RC_LIGHTS

    def isRCBreakLights(self):
        return self.buffer[0] == RC_BREAK_LIGHTS

    def isStopCar(self):
        return self.buffer[0] == RC_STOP_CAR

    def isRCMove(self):
        return self.buffer[0] == RC_MOVE

    def isRCTurn(self):
        return self.buffer[0] == RC_TURN

    def getGroup(self):
        if self.isBroadcast():
            return self.buffer[2]
        return RP_FILLER

    def getElement(self):
        if self.isAddressed() or self.isStatus():
            return self.buffer[4]
        return self.buffer[3]

    def getState(self):
        if not self.isOperation():
            if self.isAnswerState():
                return self.buffer[5]
            return RP_FILLER
        elif self.isAddressed():
            return self.buffer[5]
        return self.buffer[4]

    def getStatus(self):
        if self.isBroadcast() or self.isAddressed():
            return RP_FILLER
        return self.buffer[4]

    def getParamIdx(self):
        if self.isAddressed():
            return self.buffer[5]
        return self.buffer[4]

    def getVal0(self):
        if self.isAddressed() and not self.isStatus():
            return self.buffer[6]
        return self.buffer[5]

    def getVal1(self):
        if self.isAddressed() and not self.isStatus():
            return self.buffer[7]
        return self.buffer[6]

    def getVal2(self):
        if self.isAddressed() and not self.isStatus():
            return RP_FILLER
        return self.buffer[7]

    def getNodeNumber(self):
        if self.isBroadcast():
            return RP_FILLER
        return self.word(self.buffer[2], self.buffer[3])

    def getSender(self):
        return self.origin

    def getAction(self):
        if not self.isAction():
            return RP_FILLER
        elif self.isAddressed():
            return self.buffer[5]
        return self.buffer[4]

    def getStatusType(self):
        if not self.isStatus():
            return RP_FILLER
        return self.buffer[1]

    def getNodeId(self):
        if self.isStatus():
            return self.word(self.buffer[2], self.buffer[3])
        return self.word(self.buffer[1], self.buffer[2])

    def getAngle(self):
        return self.buffer[4]

    def getSpeed(self):
        return self.buffer[4]

    def getDirection(self):
        return self.buffer[5]

    def getServerId(self):
        return self.buffer[3]

    def getByte(self, idx):
        return self.buffer[idx]

    def createBroadcastOPMessage(self, group, element, state, val0, val1, val2):
        self.messageLength = 8
        self.buffer[0] = RP_BROADCAST
        self.buffer[1] = RP_OPERATION
        self.buffer[2] = group
        self.buffer[3] = element
        self.buffer[4] = state
        self.buffer[5] = val0
        self.buffer[6] = val1
        self.buffer[7] = val2
        return self.messageLength

    def createBroadcastRequestRegister(self, group):
        self.messageLength = 5
        self.buffer[0] = RP_BROADCAST
        self.buffer[1] = RP_ACTION
        self.buffer[2] = group
        self.buffer[3] = 255
        self.buffer[4] = RP_AC_REGISTER
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createBroadcastWriteMessage(self, group, element, param_idx, val0, val1, val2):
        self.messageLength = 8
        self.buffer[0] = RP_BROADCAST
        self.buffer[1] = RP_WRITE
        self.buffer[2] = group
        self.buffer[3] = element
        self.buffer[4] = param_idx
        self.buffer[5] = val0
        self.buffer[6] = val1
        self.buffer[7] = val2
        return self.messageLength

    def createBroadcastActionMessage(self, group, element, action, val0, val1, val2):
        self.messageLength = 8
        self.buffer[0] = RP_BROADCAST
        self.buffer[1] = RP_ACTION
        self.buffer[2] = group
        self.buffer[3] = element
        self.buffer[4] = action
        self.buffer[5] = val0
        self.buffer[6] = val1
        self.buffer[7] = val2
        return self.messageLength

    def createAddressedWriteMessage(self, nodeid, element, param_idx, val0, val1):
        self.messageLength = 8
        self.buffer[0] = RP_ADDRESSED
        self.buffer[1] = RP_WRITE
        self.buffer[2] = self.highByte(nodeid)
        self.buffer[3] = self.lowByte(nodeid)
        self.buffer[4] = element
        self.buffer[5] = param_idx
        self.buffer[6] = val0
        self.buffer[7] = val1
        return self.messageLength

    def createAddressedReadMessage(self, nodeid, element, param_idx):
        self.messageLength = 8
        self.buffer[0] = RP_ADDRESSED
        self.buffer[1] = RP_READ
        self.buffer[2] = self.highByte(nodeid)
        self.buffer[3] = self.lowByte(nodeid)
        self.buffer[4] = element
        self.buffer[5] = param_idx
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createAddressedOPMessage(self, nodeid, element, state, val0, val1):
        self.messageLength = 8
        self.buffer[0] = RP_ADDRESSED
        self.buffer[1] = RP_OPERATION
        self.buffer[2] = self.highByte(nodeid)
        self.buffer[3] = self.lowByte(nodeid)
        self.buffer[4] = element
        self.buffer[5] = state
        self.buffer[6] = val0
        self.buffer[7] = val1
        return self.messageLength

    def createAddressedActionMessage(self, nodeid, element, action, val0, val1):
        self.messageLength = 8
        self.buffer[0] = RP_ADDRESSED
        self.buffer[1] = RP_ACTION
        self.buffer[2] = self.highByte(nodeid)
        self.buffer[3] = self.lowByte(nodeid)
        self.buffer[4] = element
        self.buffer[5] = action
        self.buffer[6] = val0
        self.buffer[7] = val1
        return self.messageLength

    def createAddressedStatusMessage(self, status_code, nodeid, element, p0, p1, p2):
        self.messageLength = 8
        self.buffer[0] = RP_STATUS
        self.buffer[1] = status_code
        self.buffer[2] = self.highByte(nodeid)
        self.buffer[3] = self.lowByte(nodeid)
        self.buffer[4] = element
        self.buffer[5] = p0
        self.buffer[6] = p1
        self.buffer[7] = p2
        return self.messageLength

    def createInitialRegisterMessage(self, nodeid, status, val0, val1, val2):
        self.messageLength = 8
        self.buffer[0] = RP_STATUS
        self.buffer[1] = RP_STATUS_INITIAL_REGISTER
        self.buffer[2] = self.highByte(nodeid)
        self.buffer[3] = self.lowByte(nodeid)
        self.buffer[4] = status
        self.buffer[5] = val0
        self.buffer[6] = val1
        self.buffer[7] = val2
        return self.messageLength

    def createEmergencyBroadcast(self, group):
        self.messageLength = 5
        self.buffer[0] = RP_BROADCAST
        self.buffer[1] = RP_OPERATION
        self.buffer[2] = group
        self.buffer[3] = 255
        self.buffer[4] = STATES.EMERGENCY
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createEmergency(self, nodeid):
        self.messageLength = 6
        self.buffer[0] = RP_ADDRESSED
        self.buffer[1] = RP_OPERATION
        self.buffer[2] = self.highByte(nodeid)
        self.buffer[3] = self.lowByte(nodeid)
        self.buffer[4] = 255
        self.buffer[5] = STATES.EMERGENCY
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createBackToNormalBroadcast(self, group):
        self.messageLength = 5
        self.buffer[0] = RP_BROADCAST
        self.buffer[1] = RP_OPERATION
        self.buffer[2] = group
        self.buffer[3] = 255
        self.buffer[4] = STATES.NORMAL
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createBackToNormal(self, nodeid):
        self.messageLength = 6
        self.buffer[0] = RP_ADDRESSED
        self.buffer[1] = RP_OPERATION
        self.buffer[2] = self.highByte(nodeid)
        self.buffer[3] = self.lowByte(nodeid)
        self.buffer[4] = 255
        self.buffer[5] = STATUS.NORMAL
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createRestoreDefaultConfig(self, serverAddr, nodeid, nodeAddr):
        self.messageLength = 7
        self.buffer[0] = RP_ADDRESSED
        self.buffer[1] = RP_ACTION
        self.buffer[2] = self.highByte(nodeid)
        self.buffer[3] = self.lowByte(nodeid)
        self.buffer[4] = 255
        self.buffer[5] = STATES.AC_RESTORE_DEFAULT_PARAMS
        self.buffer[6] = serverAddr
        self.buffer[7] = 0
        return self.messageLength

    def createStatusMessage(self, serverAddr, nodeid, status):
        self.messageLength = 6
        self.buffer[0] = RP_STATUS
        self.buffer[1] = RP_STATUS_QUERY_STATUS
        self.buffer[2] = self.highByte(nodeid)
        self.buffer[3] = self.lowByte(nodeid)
        self.buffer[4] = status
        self.buffer[5] = serverAddr
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createACKMessage(self, nodeid, element, status):
        self.messageLength = 6
        self.buffer[0] = RP_STATUS
        self.buffer[1] = RP_STATUS_ANSWER_STATUS
        self.buffer[2] = self.highByte(nodeid)
        self.buffer[3] = self.lowByte(nodeid)
        self.buffer[4] = status
        self.buffer[5] = element
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createMotorOn(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.MOTOR,
                                             STATES.ON,
                                             0, 0)

    def createMotorOff(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.MOTOR,
                                             STATES.OFF,
                                             0, 0)

    def createSireneLightOn(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.SIRENE_LIGHT,
                                             STATES.ON,
                                             0, 0)

    def createSireneLightOff(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.SIRENE_LIGHT,
                                             STATES.OFF,
                                             0, 0)

    def createSireneLightBlink(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.SIRENE_LIGHT,
                                             STATES.BLINKING,
                                             0, 0)

    def createFrontLightOn(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.FRONT_LIGHT,
                                             STATES.ON,
                                             0, 0)

    def createFrontLightOff(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.FRONT_LIGHT,
                                             STATES.OFF,
                                             0, 0)

    def createFrontLightBlink(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.FRONT_LIGHT,
                                             STATES.BLINKING,
                                             0, 0)

    def createBreakLightOn(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.BREAK_LIGHT,
                                             STATES.ON,
                                             0, 0)

    def createBreakLightOff(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.BREAK_LIGHT,
                                             STATES.OFF,
                                             0, 0)

    def createBreakLightBlink(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.BREAK_LIGHT,
                                             STATES.BLINKING,
                                             0, 0)

    def createRightLightOn(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.RIGHT_LIGHT,
                                             STATES.ON,
                                             0, 0)

    def createRightLightOff(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.RIGHT_LIGHT,
                                             STATES.OFF,
                                             0, 0)

    def createRightLightBlink(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.RIGHT_LIGHT,
                                             STATES.BLINKING,
                                             0, 0)

    def createLeftLightOn(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.LEFT_LIGHT,
                                             STATES.ON,
                                             0, 0)

    def createLeftLightOff(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.LEFT_LIGHT,
                                             STATES.OFF,
                                             0, 0)

    def createLeftLightBlink(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.LEFT_LIGHT,
                                             STATES.BLINKING,
                                             0, 0)

    def createBoardDay(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.BOARD,
                                             STATES.DAY,
                                             0, 0)

    def createBoardNight(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.BOARD,
                                             STATES.NIGHT,
                                             0, 0)

    def createBoardNormal(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.BOARD,
                                             STATES.NORMAL,
                                             0, 0)

    def createBoardEmergency(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.BOARD,
                                             STATES.EMERGENCY,
                                             0, 0)

    def createBoardAllBlinking(self, nodeid):
        return self.createAddressedOPMessage(nodeid,
                                             CARPARTS.BOARD,
                                             STATES.ALL_BLINKING,
                                             0, 0)

    def createServerAutoEnum(self, nodeid):
        self.messageLength = 3
        self.buffer[0] = BR_SERVER_AUTO_ENUM
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = 0
        self.buffer[4] = 0
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createCarAutoEnum(self, nodeid):
        self.messageLength = 3
        self.buffer[0] = BR_CAR_AUTO_ENUM
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = 0
        self.buffer[4] = 0
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createNodeId(self, nodeid):
        self.messageLength = 3
        self.buffer[0] = RP_ID_RESOLUTION
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = 0
        self.buffer[4] = 0
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createCarId(self, nodeid):
        self.messageLength = 3
        self.buffer[0] = CAR_ID
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = 0
        self.buffer[4] = 0
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createRCId(self, nodeid):
        self.messageLength = 3
        self.buffer[0] = RC_ID
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = 0
        self.buffer[4] = 0
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createRCCarRegister(self, nodeid, serverid):
        self.messageLength = 4
        self.buffer[0] = RC_CAR_REGISTER
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = serverid
        self.buffer[4] = 0
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createRCCarRegisterAck(self, nodeid, serverid):
        self.messageLength = 4
        self.buffer[0] = RC_CAR_REGISTER_ACK
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = serverid
        self.buffer[4] = 0
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createAcquire(self, nodeid, serverid):
        self.messageLength = 4
        self.buffer[0] = CAR_ACQUIRE
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = serverid
        self.buffer[4] = 0
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createAcquireAck(self, nodeid, serverid):
        self.messageLength = 4
        self.buffer[0] = CAR_ACQUIRE_ACK
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = serverid
        self.buffer[4] = 0
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createAcquireNAck(self, nodeid, serverid):
        self.messageLength = 4
        self.buffer[0] = CAR_ACQUIRE_NACK
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = serverid
        self.buffer[4] = 0
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createCarRelease(self, nodeid, serverid):
        self.messageLength = 4
        self.buffer[0] = CAR_RELEASE
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = serverid
        self.buffer[4] = 0
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createCarReleaseAck(self, nodeid, serverid):
        self.messageLength = 4
        self.buffer[0] = CAR_RELEASE_ACK
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = serverid
        self.buffer[4] = 0
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createCarKeepAlive(self, nodeid, serverid):
        self.messageLength = 4
        self.buffer[0] = CAR_KEEP_ALIVE
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = serverid
        self.buffer[4] = 0
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createRCKeepAlive(self, nodeid, serverid):
        self.messageLength = 4
        self.buffer[0] = RC_KEEP_ALIVE
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = serverid
        self.buffer[4] = 0
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createSaveParam(self, nodeid, serverid, idx, value):
        self.messageLength = 6
        self.buffer[0] = SAVE_PARAM
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = serverid
        self.buffer[4] = idx
        self.buffer[5] = value
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createRCTurn(self, nodeid, serverid, angle, direction):
        self.messageLength = 6
        self.buffer[0] = RC_TURN
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = serverid
        self.buffer[4] = angle
        self.buffer[5] = direction
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createRCMove(self, nodeid, serverid, speed, direction):
        self.messageLength = 6
        self.buffer[0] = RC_MOVE
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = serverid
        self.buffer[4] = speed
        self.buffer[5] = direction
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createCarLightOnOff(self, nodeid, serverid):
        self.messageLength = 4
        self.buffer[0] = RC_LIGHTS
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = serverid
        self.buffer[4] = 0
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createCarBreakLightOnOff(self, nodeid, serverid):
        self.messageLength = 4
        self.buffer[0] = RC_BREAK_LIGHTS
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = serverid
        self.buffer[4] = 0
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createLowBattery(self, serverid, nodeid):
        self.messageLength = 7
        self.buffer[0] = RP_ADDRESSED
        self.buffer[1] = RP_ACTION
        self.buffer[2] = self.highByte(nodeid)
        self.buffer[3] = self.lowByte(nodeid)
        self.buffer[4] = 255
        self.buffer[5] = ACTIONS.AC_LOWBATTERY
        self.buffer[6] = serverid
        self.buffer[7] = 0
        return self.messageLength

    def createStopCar(self, nodeid, serverid):
        self.messageLength = 4
        self.buffer[0] = RC_STOP_CAR
        self.buffer[1] = self.highByte(nodeid)
        self.buffer[2] = self.lowByte(nodeid)
        self.buffer[3] = serverid
        self.buffer[4] = 0
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createQueryState(self, nodeid, element):
        self.messageLength = 5
        self.buffer[0] = RP_STATUS
        self.buffer[1] = RP_STATUS_QUERY_STATE
        self.buffer[2] = self.highByte(nodeid)
        self.buffer[3] = self.lowByte(nodeid)
        self.buffer[4] = element
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createQueryAllStates(self, nodeid):
        self.messageLength = 4
        self.buffer[0] = RP_STATUS
        self.buffer[1] = RP_STATUS_QUERY_ALL_STATES
        self.buffer[2] = self.highByte(nodeid)
        self.buffer[3] = self.lowByte(nodeid)
        self.buffer[4] = 0
        self.buffer[5] = 0
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def createAnswerState(self, nodeid, element, state):
        self.messageLength = 6
        self.buffer[0] = RP_STATUS
        self.buffer[1] = RP_STATUS_QUERY_STATE
        self.buffer[2] = self.highByte(nodeid)
        self.buffer[3] = self.lowByte(nodeid)
        self.buffer[4] = element
        self.buffer[5] = state
        self.buffer[6] = 0
        self.buffer[7] = 0
        return self.messageLength

    def dumpBuffer(self):
        self.logger.debug("CSRD buffer: %X %X %X %X %X %X %X %X",
                          self.buffer[0],
                          self.buffer[1],
                          self.buffer[2],
                          self.buffer[3],
                          self.buffer[4],
                          self.buffer[5],
                          self.buffer[6],
                          self.buffer[7])

    def resetToDefault(self):
        self.nodenumber = 999
        self.params[RP_PARAM_GROUP] = 1
        self.params[RP_PARAM_BATTERY_THRESHOLD] = 30
        self.params[RP_PARAM_SOUND_ON] = RP_OFF
        self.params[RP_PARAM_SPEED_STEP] = 5
        self.params[RP_PARAM_SPEED_COMPENSATION] = RP_ON
        self.params[RP_PARAM_FREQUENCY] = 2  # 0 = 433 MHz 1 = 868 MHz 2 = 915 MHz
        self.params[RP_PARAM_BREAK_RATE] = 20  # 0 = 433 MHz 1 = 868 MHz  2 = 915 MHz

    def bufferToHexString(self):
        s = "%0.2X%0.2X%0.2X%0.2X%0.2X%0.2X%0.2X%0.2X" % (self.buffer[0],
                                                          self.buffer[1],
                                                          self.buffer[2],
                                                          self.buffer[3],
                                                          self.buffer[4],
                                                          self.buffer[5],
                                                          self.buffer[6],
                                                          self.buffer[7])
        return s

    def getNiceMessage(self):

        s = ""
        try:
            if self.isAddressed():
                s = "Addressed"
                if self.isAction():
                    s += " Action"
                elif self.isRead():
                    s += " Read"
                elif self.isWrite():
                    s += " Write"
                elif self.isOperation():
                    s += " Operation"
                else:
                    s += " Unknown"

                s += " to node " + str(self.getNodeId())
                s += " element " + self.elements[self.getElement()]
                if self.isOperation():
                    s += " next state " + self.states[self.getState()]
                elif self.isAction():
                    s += " Action " + self.getAction()
                else:
                    s += " Param idx " + self.getParamIdx()
            elif self.isBroadcast():
                s = "Broadcast"
                if self.isAction():
                    s += " Action"
                elif self.isRead():
                    s += " Read"
                elif self.isWrite():
                    s += " Write"
                elif self.isOperation():
                    s += " Operation"
                else:
                    s += " Unknown"

                s += " to group " + str(self.getGroup())
                s += " element " + self.elements[self.getElement()]
                if self.isOperation():
                    s += " next state " + self.states[self.getState()]
                elif self.isAction():
                    s += " Action " + self.getAction()
                    if self.isBroadcastRegister():
                        s += " Broadcast register"
                else:
                    s += " Param idx " + self.getParamIdx()
            elif self.isStatus():
                s += "Status"
                if self.isInitialRegister():
                    s += " Initial Registration"
                    s += " " + str(self.getNodeId())
                    s += " " + str(self.getStatus())
                elif self.isAnswerState():
                    s += " answer to node " + str(self.getNodeId())
                    s += " element " + self.elements[self.getElement()]
                    s += " state " + self.states[self.getState()]
                elif self.isQueryAllStates():
                    s += " to node " + str(self.getNodeId())
                    s += " query all states"
                elif self.isQueryState():
                    s += " query to node " + str(self.getNodeId())
                    s += " element " + self.elements[self.getElement()]
                    s += " next state " + self.states[self.getState()]
                else:
                    s += "other state"
        except Exception as e:
            s = f"Failed to print nice {s} error {e}"

        return s

    def printNice(self):
        self.logger.debug(self.getNiceMessage())

    def bufferToJson(self):
        return ""

    def word(self, a, b):
        combined = a << 8 | b
        return combined

    def lowByte(self, value):
        b = value & 0x00ff
        return b

    def highByte(self, value):
        b = value >> 8
        return b