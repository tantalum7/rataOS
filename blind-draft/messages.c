/*******************************************************************************
* RataOS Task Scheduler
* File 				: messages.c
* Description   	: Provides the framework for the OS messaging system.
* Revision History	: Unreleased
* License			: Eclipse Public License
*					  http://www.opensource.org/licenses/eclipse-1.0.php
* Author			: Oliver Kent
* Project Location	: http://rataos.sourceforge.net/
*******************************************************************************/

/* System Parameters */
#define MAX_MESSAGES_ROS				32u
#define MESSAGE_STORAGE_BYTES_ROS		256u
#define MAX_MESSAGE_BYTES_ROS			32u

/* API Control Parameters */
#define GLOBAL_MESSAGE_TARGET_ROS		0x00

/* Error Return Codes */
#define F_MESSAGE_ID_OCCUPIED_ROS		0x25
#define F_MESSAGE_ID_EMPTY_ROS			0x26
#define F_MESSAGE_ACCESS_VIOLATION_ROS	0x27
#define F_MAX_MESSAGES_REACHED_ROS		0x28

/* Message data storage array */
uint8_t gMessageStorageArray_ROS[MESSAGE_STORAGE_BYTES];

/* Message ID lookup table */
uint8_t gMessageIDLookupArray_ROS[MAX_MESSAGE_VECTOR];

/* Message location index array */
uint8_t gMessageLocationArray_ROS[MAX_MESSAGES];

/* Message size index array */
uint8_t gMessageSizeArray_ROS[MAX_MESSAGES];

/* Message target vector array */
uint8_t gMessageTargetArray_ROS[MAX_MESSAGES];

/* Message time remaining to live array */
uint8_t gMessageAliveTimeArray_ROS[MAX_MESSAGES];

/* Unread message flag for each vector, array */
uint8_t gMessageUnreadArray_ROS[MAX_TASKS];

/* Next free message location global variable */
uint8_t gMessageLocationStack_ROS = 0u;

/* Next free message index number */
uint8_t gMessageIndexStack_ROS = 0u;

/* Number of messages global variable */
uint8_t gNumberMessages_ROS = 0u;

extern uint8_t gCurrentOperatingTask;

/* TIME TO LIVE!!! */


/*******************************************************************************
* Name			: CreateMessage_ROS
* Description	: xxx
* Notes			: None.
*******************************************************************************/
uint8_t CreateMessage_ROS
		(
			uint8_t target_vector, /
			uint8_t message_id, /
			uint8_t time_to_live, /
			uint8_t number_bytes, /
			uint8_t * pointer_to_message
		)
{
	/* Check if message target vector is within range */
	if(target_vector > MAX_TASKS_ROS)
	{
		/* Task vector too high, return failure */
		return F_TASK_VECTOR_TOO_HIGH_ROS;
	}
	/* Check if message target is a valid task */
	else if(*gTaskPointerArray_RS[target_vector] == NULL_POINTER_ROS)
	{
		/* Task vector empty, return failure */
		return F_TASK_VECTOR_EMPTY_ROS;
	}			
	/* Check if message id is unused */
	else if(gMessageIDLookupArray_ROS[message_id] != NULL_VALUE_ROS)
	{
		/* Task id is already in use, return failure */
		return F_MESSAGE_ID_OCCUPIED_ROS;
	}
	/* Check if maximum number of messages have been reached */
	else if(gNumberMessages == MAX_MESSAGES_ROS)
	{
		/* Maximum number of messages reached, return failure */
		return F_MAX_MESSAGES_REACHED_ROS;
	}
	/* Check if message size is within range */
	else if((!number_bytes) | (number_bytes > MAX_MESSAGES_ROS))
	{
		/* Message size too big (or zero), return failure */
		return F_MESSAGE_SIZE_TOO_LARGE_ROS;
	}
	/* Check if there is sufficent space for message */
	else if((gMessageMemoryTop_ROS + number_bytes) > \
	         MESSAGE_STORAGE_BYTES_ROS)
	{
		/* Insufficent message space, return failure */
		return F_INSUFFICENT_MESSAGE_SPACE_ROS;
	}
	/* Valid message post request, proceed */
	else
	{
		/* Declare message index variable, set to the next free index number */
		uint8_t message_index = gMessageIndexStack_ROS;
		
		/* Declare message location variable, set to the next free location */
		uint8_t message_location = gMessageLocationStack_ROS;
		
		/* Store message in message memory space */
		memmove(gMessageStorageArray_ROS[message_location], \
				pointer_to_message, number_bytes);
				
		/* Store message ID in array */
		gMessageLookupIDArray_ROS[message_id] = message_index;
		
		/* Store number of message bytes in message size array */
		gMessageSizeArray_ROS[message_index] = number_bytes;
		
		/* Store message target array */
		gMessageTargetArray_ROS[message_index] = target_vector;
		
		/* Check if message target is not global */
		if(target_vector != GLOBAL_MESSAGE_TARGET_ROS)
		{
			/* Inform target of unread message */
			gMessageUnreadArray_ROS[taget_vector]++;
		}
		
		/* Move message stack position to next free location */
		gMessageLocationStack_ROS += number_bytes;
		
		/* Increment the message index to the next free value */
		gMessageIndexStack_ROS++;
		
		/* Increment number of global messages */
		gNumberMessages_ROS++;
		
		/* Message posting complete, return success */
		return SUCCESS_ROS;
	}
}
/*******************************************************************************
* End of CreateMessage_ROS
*******************************************************************************/

/*******************************************************************************
* Name			: ReadMessage_ROS
* Description	: xxx
* Notes			: None.
*******************************************************************************/
uint8_t ReadMessage_ROS
		(
			/* Target message ID */
			uint8_t	message_id, \
			/* Pointer to array to contain message output */
			uint8_t * message_output_pointer, \
			/* Maximum number of bytes to read */
			uint8_t max_length
		)
{
	/* Check if message ID is valid */
	if(message_id > MAX_MESSAGE_VECTOR_ROS)
	{
		/* Message ID too high, return failure */
		return F_MESSAGE_ID_TOO_HIGH_ROS;
	}
	/* Message ID is valid */
	else
	{
		/* Declare variable and set it to the message index */
		uint8_t message_index = gMessageIDLookupArray_ROS[message_id];
	
		/* Declare variable and set it to the message target */
		uint8_t message_target = gMessageTargetArray_ROS[message_index];
	
		/* Declare variable and set it to message length */
		uint8_t message_length = gMessageSizeArray_ROS[message_index];
	
		/* Declare variable and set it to the current OS task */
		uint8_t os_task = gCurrentOperatingTask_ROS;
			
		/* Check if message ID contains a valid message */
		if(message_index == NULL_VALUE_ROS)
		{
			/* Message ID does not contain a valid message, return failure */
			return F_MESSAGE_ID_EMPTY_ROS;
		}	
		/* Check if current task owns specified message, or message is global */
		else if((message_target != os_task) | \
				(message_target != GLOBAL_MESSAGE_TARGET_ROS))
		{
			/* Message does not belong to task, and is not global.
			   Return failure */
			return F_MESSAGE_ACCESS_VIOLATION_ROS;
		};
		/* Message ID valid and is owned by current task, proceed to read */
		else
		{
			/* Copy maximum requested bytes unless message length is less */
			uint8_t bytes_to_read = max_length > messgage_length ? \
									message_length : max_length;	
		
			/* Copy the message to the output pointer */
			strncpy(message_output_pointer, message_output_pointer, \
					bytes_to_read);
		
			/* Decrement the unread messages count for the current task */
			gMessageUnreadArray_ROS[message_index]--;
		
			/* Message read complete, return success */
			return SUCCESS_ROS;
		}
	}
}
/*******************************************************************************
* End of ReadMessage_ROS
*******************************************************************************/	
		
		
