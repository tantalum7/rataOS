#include <stdint.h>

#ifndef MESSAGES_H
#define MESSAGES_H


/* System Parameters */
#define MAX_MSGS_ROS						32u
#define MAX_MSG_STOR_BYTES_ROS				256u
#define MIN_MSG_BYTES_ROS					1u
#define MAX_MSG_BYTES_ROS					32u
#define MAX_DEL_MSGS_ROS					8u
#define	MIN_MSG_ID_ROS						0x02
#define MAX_MSG_ID_ROS						0xF0
#define MAX_MSG_ATTR_ROS					5u
#define MAX_DEL_MSG_ATTR_ROS 				6u


/* Imported */
#define MAX_TASKS_ROS						32u
#define SUCCESS_ROS							0x01
#define TRUE_ROS							0x02
#define FALSE_ROS							0x03

/* Misc */
#define NULL_ID_ROS							0u
#define NULL_MSG_ROS						0u
#define NULL_LOC_ROS						0u
#define NULL_SIZE_ROS						0u
#define NULL_TTL_ROS						0u
#define NULL_TARG_ROS						0u
#define MSG_ID_ROS							0u
#define MSG_SIZE_ROS						1u
#define MSG_LOC_ROS							2u
#define MSG_TARG_ROS						3u
#define MSG_TTL_ROS							4u
#define MSG_OLDINDEX_ROS					5u

/* API Control Parameters */
#define MSG_TARG_GLOBAL_ROS					0x00

/* Error Return Codes */
#define F_MSG_ID_OCCUPIED_ROS				0x25
#define F_MSG_ID_EMPTY_ROS					0x26
#define F_MSG_ACCESS_DENIED_ROS				0x27
#define F_MAX_MSGS_REACHED_ROS				0x28
#define F_MSG_ID_TOO_HIGH_ROS				0x29
#define F_MSG_ID_TOO_LOW_ROS				0x30
#define F_MSG_SIZE_TOO_LARGE_ROS			0x31
#define F_MSG_SIZE_TOO_LOW_ROS				0x32
#define F_MAX_MSGS_REACHES_ROS				0x33
#define F_INSUFF_MEM_SPACE_ROS				0x34
#define F_INSUFF_FREE_MEM_ROS				0x35
#define F_MAX_DEL_MSGS_REACHED_ROS			0x36


extern uint8_t gMsgTable_ROS[MAX_MSGS_ROS][MAX_MSG_ATTR_ROS];
extern uint8_t gDelMsgTable_ROS[MAX_DEL_MSGS_ROS][MAX_DEL_MSG_ATTR_ROS];
extern uint8_t gMessageIDLookupArray_ROS[MAX_MSG_ID_ROS];
extern uint8_t gNumDelBytes_ROS;
#endif