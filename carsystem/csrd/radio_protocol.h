#ifndef __RADIO__PROTOCOL__
#define __RADIO__PROTOCOL__

#define RP_BROADCAST    0xff
#define RP_ADDRESSED    0
#define RP_STATUS       1

#define RP_WRITE        0
#define RP_OPERATION    1
#define RP_READ         2
#define RP_ACTION       3

#define RP_INITIALREG   0

#define PARAMETERS_SIZE         11

#define RP_AC_BLINK_LEFT        1
#define RP_AC_BLINK_RIGHT       2
#define RP_AC_FRONT_LIGHT       3
#define RP_AC_REAR_LIGHT        4
#define RP_AC_BREAK_LIGHT       5
#define RP_AC_SIRENE_LIGHT      6
#define RP_AC_BREAK             7
#define RP_AC_ACCELERATE        8
#define RP_AC_GET_ALL_PARAMS    9

#define RP_PARAM_GROUP              1
#define RP_PARAM_BATTERY_THRESHOLD  2
#define RP_PARAM_SOUND_ON           3
#define RP_PARAM_SPEED_STEP         4
#define RP_PARAM_SPEED_COMPENSATION 5
#define RP_PARAM_FREQUENCY          6
#define RP_PARAM_BREAK_RATE         7


#define RP_ON           1
#define RP_OFF          2
#define RP_FILLER       0xfe


/*message format
8 bytes
<A><B><C><D><E><F><G><H>

A indicates if it is a broadcast message (RP_BROADCAST) or an addressed message (RP_ADDRESSED) or status messages ( RP_STATUS normally from client to server)

If Broadcast
    B indicates the type of action: store value (RP_WRITE) or perform some operation RP_OPERATION
    C is the group. If equal to 0 means all groups
    D is the element. .0xff is the board element else (motor, left light,sirene, aux0, aux1, etc..)

        Case store value
            E is param index
            F G H are values

        Case Operation
            E is the next state
            F G H are general values
        case action
            F is the action
            G is the action param

If addressed message
    B indicates the type of action: store value (RP_WRITE), perform some operation RP_OPERATION or read parameter RP_READ
    C and D is the node id
    E is the element .0xff is the board element else is (motor, left light,sirene, aux0, aux1, etc..)
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
            G is the action param

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

Action messages

*/


#endif // __RADIO__PROTOCOL__

