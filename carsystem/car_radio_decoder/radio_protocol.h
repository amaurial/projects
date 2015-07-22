#ifndef __RADIO__PROTOCOL__
#define __RADIO__PROTOCOL__

#define RP_BROADCAST    255
#define RP_ADDRESSED    0
#define RP_WRITE        1
#define RP_OPERATION    2
#define RP_READ         3

#define RP_OP_BLINK_LEFT        1
#define RP_OP_BLINK_RIGHT       2
#define RP_OP_FRONT_LIGHT       3
#define RP_OP_REAR_LIGHT        4
#define RP_OP_BREAK_LIGHT       5
#define RP_OP_SIRENE_LIGHT      6
#define RP_OP_BREAK             7
#define RP_OP_ACCELERATE        8
#define RP_OP_GET_ALL_PARAMS    9

#define RP_ON           1
#define RP_OFF          2
#define RP_FILLER       254

#define RP_PARAM_ID                 0
#define RP_PARAM_GROUP              1
#define RP_PARAM_BLINK_RATE         2
#define RP_PARAM_NORMAL_SPEED       3
#define RP_PARAM_CURR_SPEED         4

#define RP_PARAM_BATTERY_THRESHOLD  6   // the content is in percentage
#define RP_PARAM_SOUND_ON           7
#define RP_PARAM_SPEED_STEP         8
#define RP_PARAM_SPEED_COMPENSATION 9
#define RP_PARAM_FREQUENCY          10


/*message format
8 bytes
<A><B><C><D><E><F><G><H>

A indicates if it is a broadcast message (RP_BROADCAST) or an addressed message (RP_ADDRESSED)

If Broadcast
    B indicates the type of action: store value (RP_WRITE) or perform some operation RP_OPERATION

        Case store value
            C is the group. If equal to 0 means all groups
            D is code of parameter
            E to H are values

        Case Operation
            C is the group. If equal to 0 means all groups
            D is the operation code
            E to H are the parameters

If addressed message
    B indicates the type of action: store value (RP_WRITE), perform some operation RP_OPERATION or read parameter RP_READ
    C and D is the node id

        Case write
            E is code of parameter
            F to H are values
        Case read
            E is code of parameter


*/


#endif // __RADIO__PROTOCOL__

