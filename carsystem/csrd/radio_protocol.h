#ifndef __RADIO__PROTOCOL__
#define __RADIO__PROTOCOL__

#define RP_BROADCAST        0xff
#define RP_ADDRESSED        0
#define RP_STATUS           1
#define RP_ID_RESOLUTION    2
#define BR_SERVER_AUTO_ENUM 3
#define BR_CAR_AUTO_ENUM    4
#define RC_ID               5
#define CAR_ID              6
#define CAR_ACQUIRE         7
#define CAR_RELEASE         8
#define CAR_ACQUIRE_ACK     9
#define CAR_RELEASE_ACK     10
#define CAR_KEEP_ALIVE      11
#define RC_KEEP_ALIVE       12
#define SAVE_PARAM          13

#define RP_WRITE        0 //write data to eprom
#define RP_OPERATION    1 //controls the changes in states
#define RP_READ         2 //read parameter
#define RP_ACTION       3 //set values to elements on runtime. data is not saved to eprom

//status messages
#define RP_INITIALREG          0
#define RP_REPORT_STATUS       1
#define RP_REPORT_ACK          2

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
#define RP_AC_REGISTER          10

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
    B indicates the type of action: store value (RP_WRITE), perform some operation RP_OPERATION or read parameter RP_READ, RP_ACTION
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
        E = element
        F = status
Action messages

*/


#endif // __RADIO__PROTOCOL__

