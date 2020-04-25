"""
message format
8 bytes
<A><B><C><D><E><F><G><H>

"A" indicates if it is a broadcast message (RP_BROADCAST) or an addressed message (RP_ADDRESSED) or status messages RP_STATUS

If Broadcast
    B indicates the type of action: store value (RP_WRITE) or perform some operation RP_OPERATION, RP_ACTION
    C is the group. If equal to 0 means all groups
    D is the element. .0xff is the board element else (motor, left light,sirene, aux0, aux1, etc..)

        Case store value
            E is param index
            F G H are values

        Case Operation
            E is the next state
            F G H are general values

        case action
            E is the action
            F,G,H is the action param

If addressed message
    B indicates the type of action:
      store value (RP_WRITE), perform some operation
      RP_OPERATION or read parameter RP_READ, RP_ACTION
    C and D is the node id
    E is the element .0xff is the board element else is
      (motor, left light,sirene, aux0, aux1, etc..)
        Case write
            F is param index
            G H are values
        Case read
            F is param index . 0xff indicate return all parameters
        case operation
            F is the next state
            G H are a general value
        case action
            F is the action
            G,H is the action param


Status messages
    Initial register message
        B = 0x00
        C and D = node id
        F = status (ACTIVE,INACTIVE,CHARGING,PANNE)
        G,H = optional values
    Query status
        B = 0x01
        C and D = node id
        F = status (ACTIVE,INACTIVE,CHARGING,PANNE)
        G,H = optional values
    Answer status
        B = 0x02
        C and D = node id
        F = status (ACTIVE,INACTIVE,CHARGING,PANNE)
        G,H = optional values
    Query value
        B=0x03
        C and D = node id
        E=element
	F=param1
	G=param2
	H=param3
    Answer query
        B=0x04
	C and D = node id
        E=element
	F=param1 value
	G=param2 value
	H=param3 value
    ACK
        B=0x04
        C and D = node id
        E = status
        F = element
Action messages

"""

MESSAGE_SIZE = 8

# message types

RP_BROADCAST = 255
RP_ADDRESSED = 0
RP_STATUS = 1
RP_ID_RESOLUTION = 2
RP_UNKOWN = 254
BR_SERVER_AUTO_ENUM = 3
BR_CAR_AUTO_ENUM = 4
RC_ID = 5
CAR_ID = 6
CAR_ACQUIRE = 7
CAR_RELEASE = 8
CAR_ACQUIRE_ACK = 9
CAR_RELEASE_ACK = 10
CAR_KEEP_ALIVE = 11
RC_KEEP_ALIVE = 12
SAVE_PARAM = 13
RC_LIGHTS = 14
RC_BREAK_LIGHTS = 15
RC_STOP_CAR = 16
RC_MOVE = 17
RC_TURN = 18
CAR_ACQUIRE_NACK = 19
RC_CAR_REGISTER = 20
RC_CAR_REGISTER_ACK = 21

RP_WRITE = 0 #write data to eprom
RP_OPERATION = 1 #controls the changes in states
RP_READ = 2 #read parameter
RP_ACTION = 3 #set values to elements on runtime. data is not saved to eprom

# status messages
RP_INITIALREG = 0
RP_REPORT_STATUS = 1
RP_REPORT_ACK = 2

PARAMETERS_SIZE = 11

RP_AC_BLINK_LEFT = 1
RP_AC_BLINK_RIGHT = 2
RP_AC_FRONT_LIGHT = 3
RP_AC_REAR_LIGHT = 4
RP_AC_BREAK_LIGHT = 5
RP_AC_SIRENE_LIGHT = 6
RP_AC_BREAK = 7
RP_AC_ACCELERATE = 8
RP_AC_GET_ALL_PARAMS = 9
RP_AC_REGISTER = 10

RP_PARAM_GROUP = 1
RP_PARAM_BATTERY_THRESHOLD = 2
RP_PARAM_SOUND_ON = 3
RP_PARAM_SPEED_STEP = 4
RP_PARAM_SPEED_COMPENSATION = 5
RP_PARAM_FREQUENCY = 6
RP_PARAM_BREAK_RATE = 7


RP_ON = 1
RP_OFF = 2
RP_FILLER = 254