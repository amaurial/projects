#ifndef __RADIO__PROTOCOL__
#define __RADIO__PROTOCOL__

#define RP_BROADCAST    0xff
#define RP_ADDRESSED    0x01
#define RP_WRITE        0x01
#define RP_OPERATION    0x02
#define RP_READ         0x03

#define RP_OP_BLINK_LEFT
#define RP_OP_BLINK_RIGHT
#define RP_OP_FRONT_LIGHT
#define RP_OP_REAR_LIGHT
#define RP_OP_BREAK_LIGHT
#define RP_OP_SIRENE_LIGHT
#define RP_OP_BREAK
#define RP_OP_ACCELERATE

#define RP_ON           0x01
#define RP_OFF          0x00

#define RP_PARAM_BLINK_RATE
#define RP_PARAM_NORMAL_SPEED
#define RP_PARAM_ID
#define RP_PARAM_CURR_SPEED
#define RP_PARAM_GROUP
#define RP_PARAM_

#endif // __RADIO__PROTOCOL__

