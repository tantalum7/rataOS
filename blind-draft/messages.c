#include <stdint.h>
#include <stdbool.h>
#include "messages.h"



/* Message data storage array */
uint8_t gMsgStorArray_ROS[MAX_MSG_STOR_BYTES_ROS];

/* Message packet table */
uint8_t gMsgTable_ROS[MAX_MSGS_ROS][MAX_MSG_ATTR_ROS];

/* Deleted message packet table */
uint8_t gDelMsgTable_ROS[MAX_DEL_MSGS_ROS][MAX_DEL_MSG_ATTR_ROS];

/* Message ID lookup table */
uint8_t gMsgIndexArray_ROS[MAX_MSG_ID_ROS];

/* Next free message location global variable */
uint8_t gNextFreeMsgLoc_ROS = 1u;

/* Next free message index number */
uint8_t gNextFreeMsgIndex_ROS = 1u;

/* Number of messages global variable */
uint8_t gNumMsg_ROS = 0u;

/* Total number of deleted messages */
uint8_t gNumDelMsg_ROS = 0u;

/* Total number of deleted bytes */
uint8_t gNumDelBytes_ROS = 0u;





uint8_t _IsMessageOwner_ROS(uint8_t);
uint8_t _IsMessageIDValid_ROS(uint8_t);
uint8_t _IsMessageIDEmpty_ROS(uint8_t);
uint8_t _IsMessageSizeValid_ROS(uint8_t);
uint8_t _FindMsgSpace_ROS(uint8_t, uint8_t *, uint8_t *, bool *, uint8_t *);


void _EraseMsgEntry_ROS(uint8_t);
void _EraseDelMsgEntry_ROS(uint8_t);



uint8_t CreateMessage_ROS (uint8_t, uint8_t, uint8_t, uint8_t, uint8_t *);
uint8_t DeleteMessage_ROS (uint8_t);

/***************************************************************************************************
* Name			: CreateMessage_ROS
* Type			: API function, message system
* Description	: This function creates new messages at the API level. The function will return
*				  prematurely if input validation fails with an error code. The following conditions
*				  will cause the function to fail:
*					- Message ID already occupied
*					- Any of the parameters out of range (see message.h for limits).
*					- Insufficent memory space
*					- Insufficent available memory space (defragmentation required).
*					- Maximum number of messages reached (see message.h for maximum).
*				  If the function succeeds, the message data is either stored in a deleted message's
*				  location, or ontop of the last created message. The details of the message are 
*				  stored in gMsgTable_ROS, and a lookup entry inserted into gMsgIndexArray_ROS. 
* Notes			: 1. If this function is interrupted by any other message function, the created 
*					 message may be corrupted.
*				  2. Time to live and message targets have not been fully implemented, although they
*				     are enterted into gMsgTable_ROS.
* Developer NB	: Develop a FastCreateMessage_ROS function that always puts the message ontop of the
*				  last (bypass looking for deleted locations)?
***************************************************************************************************/		
uint8_t CreateMessage_ROS
		(
			/* Desired ID for new message */
			uint8_t message_id, \
			/* Target task vector to address message to */
			uint8_t target_vector, \
			/* New messages maximum time to live */
			uint8_t time_to_live, \
			/* Size of the new message in bytes */
			uint8_t message_size, \
			/* Pointer to the message data */
			uint8_t * pointer_to_message
		)
{
	/* Declare input validation result container variables */
	uint8_t id_empty, message_size_valid;

	/* Check if the message ID is valid and if it is empty with the IsIDEmpty function, store result
	   in container variable */
	id_empty = _IsMessageIDEmpty_ROS(message_id);

	/* Check if the message size is in range, and store result in container variable */
	message_size_valid = _IsMessageSizeValid_ROS(message_size);

	/* Check if empty message check returned false */
	if(id_empty == FALSE_ROS)
	{
		/* Message ID is already occupied, return failure */
		return F_MSG_ID_OCCUPIED_ROS;
	}
	/* Check if empty message check did not return true or false (must be error code) */
	else if(id_empty != TRUE_ROS)
	{
		/* Message ID invalid, return failure */
		return id_empty;
	}
	/* Check if message size valid check did not return true (must be error code) */
	else if(message_size_valid != TRUE_ROS)
	{
		/* Message size invalid, return failure */
		return message_size_valid;
	}
	/* Input validation successful, proceed to create message */
	else
	{
		/* Declare space found result container variable */
		uint8_t is_space_found;

		/* Declare variables to contain the found space's location, index and old deleted index */
		uint8_t message_location, message_index, deleted_message_index;

		/* Declare flag to signal whether location found is a deleted location (true = yes) */
		bool is_deleted_location;

		/* Call the FindMsgSpace internal function, and store the found space's details in the 
		   container variables. Store the functions return code in the is_space_found variable */
		is_space_found = _FindMsgSpace_ROS
						(
							message_size, \
							&message_location, \
							&message_index, \
							&is_deleted_location, \
							&deleted_message_index
						);
		/* Check the is_space_found variable, to see if the find space operation was successful */
		if(is_space_found == SUCCESS_ROS)
		{
			/* Store the new message index into the ID -> index lookup table */
			gMsgIndexArray_ROS[message_id] = message_index;

			/* Store the message parameteres in the message table */
			gMsgTable_ROS[message_index][MSG_ID_ROS] = message_id;
			gMsgTable_ROS[message_index][MSG_SIZE_ROS] = message_size;
			gMsgTable_ROS[message_index][MSG_LOC_ROS] = message_location;
			gMsgTable_ROS[message_index][MSG_TTL_ROS] = time_to_live;
			gMsgTable_ROS[message_index][MSG_TARG_ROS] = target_vector;

			/* Copy the message data from the passed pointer into the new message location */
			memmove(&gMsgStorArray_ROS[message_location], pointer_to_message, message_size);		

			/* Check the is_deleted_location flag, to check if the new message is in a deleted 
			   location */
			if(is_deleted_location)
			{
				/* Decrease the number of deleted bytes by the size of the message now created */
				gNumDelBytes_ROS -= message_size;

				/* Decrement the total number of deleted messages by one */
				gNumDelMsg_ROS--;

				/* Call the erase function to erase the parameters of the deleted message from the
				   deleted message table */
				_EraseDelMsgEntry_ROS(deleted_message_index);
			}
			/* New message was not created in a deleted message location */
			else
			{
				/* Increase the next free message location by the number of bytes of the new message
				   (the next free location) */
				gNextFreeMsgLoc_ROS += message_size;

				/* Increment the next free message index by one (the next free index) */
				gNextFreeMsgIndex_ROS++;
			}

			/* Increase the total number of messages by one */
			gNumMsg_ROS++;

			/* Message created, return success */
			return SUCCESS_ROS;
		}
		/* Could not find space for the new message */
		else
		{
			/* Cannot find space for new message, is_space_found contains the error code detailing
			   why the find space operation failed. Return failure */
			return is_space_found;
		}
	}
	
	/* DEV: Code should never reach this point. Do we need a return value? */
	return 0;
}
/***************************************************************************************************
* End of CreateMessage_ROS
***************************************************************************************************/


uint8_t DeleteMessage_ROS
		(
			uint8_t message_id
		)
{
	uint8_t is_id_empty;
	
	is_id_empty = _IsMessageIDEmpty_ROS(message_id);
	
	if(is_id_empty == TRUE_ROS)
	{
		return F_MSG_ID_EMPTY_ROS;
	}
	else if(is_id_empty != FALSE_ROS)
	{
		return is_id_empty;
	}
	else
	{
		uint8_t i, del_index;
		
		bool found_del_slot = false;
		
		for(i = 0; i < MAX_DEL_MSGS_ROS; i++)
		{
			if(gDelMsgTable_ROS[i][MSG_ID_ROS] == NULL_ID_ROS)
			{
				/* Found free deleted messages slot */
				del_index = i;
				
				found_del_slot = true;
				
				break;
			}
			
		}
		
		if(!found_del_slot)
		{
			/* Need to defrag, no deleted table space left */
			return F_MAX_DEL_MSGS_REACHED_ROS;
		}
		else
		{
			uint8_t message_index = gMsgIndexArray_ROS[message_id];
			
			gMsgIndexArray_ROS[message_id] = NULL_ID_ROS;
			
			gNumDelMsg_ROS++;
			
			gNumDelBytes_ROS += gMsgTable_ROS[message_index][MSG_SIZE_ROS];
			
			gDelMsgTable_ROS[del_index][MSG_ID_ROS]		  = gMsgTable_ROS[message_index][MSG_ID_ROS];
	 		gDelMsgTable_ROS[del_index][MSG_SIZE_ROS] 	  = gMsgTable_ROS[message_index][MSG_SIZE_ROS];
			gDelMsgTable_ROS[del_index][MSG_LOC_ROS] 	  = gMsgTable_ROS[message_index][MSG_LOC_ROS];
			gDelMsgTable_ROS[del_index][MSG_TTL_ROS]      = gMsgTable_ROS[message_index][MSG_TTL_ROS];
			gDelMsgTable_ROS[del_index][MSG_TARG_ROS] 	  = gMsgTable_ROS[message_index][MSG_TARG_ROS];
			gDelMsgTable_ROS[del_index][MSG_OLDINDEX_ROS] = message_index;
					
			_EraseMsgEntry_ROS(message_index);
			
			return SUCCESS_ROS;
		}
	}

}







uint8_t _FindMsgSpace_ROS
		(
			uint8_t message_size, \
			uint8_t * output_location, \
			uint8_t * message_index, \
			bool * is_deleted_location, \
			uint8_t * deleted_message_index 
		)
{
	if(gNumMsg_ROS == MAX_MSGS_ROS)
	{
		return F_MAX_MSGS_REACHED_ROS;
	}
	else
	{
		bool space_found = false;
	
		/* Check if there is a suitable deleted memory location */
		if(gNumDelBytes_ROS >= message_size)
		{
			uint8_t i;
	
			/* Check deleted messages table */
			for(i = 0u; i < MAX_DEL_MSGS_ROS; i++)
			{
				if(gDelMsgTable_ROS[i][MSG_SIZE_ROS] >= message_size)
				{
					*output_location = gDelMsgTable_ROS[i][MSG_LOC_ROS];
	
					*is_deleted_location = true;
				
					*deleted_message_index = i;
				
					*message_index = gDelMsgTable_ROS[i][MSG_OLDINDEX_ROS];
				
					space_found = true;
				
					break;
				}
			}
		}
	
		if(!space_found)
		{
			if((gNextFreeMsgLoc_ROS + message_size) < MAX_MSG_STOR_BYTES_ROS)
			{
				*output_location = gNextFreeMsgLoc_ROS;
			
				*message_index = gNextFreeMsgIndex_ROS;
				
				*is_deleted_location = false;
				
				return SUCCESS_ROS;
			}
			else
			{
				return F_INSUFF_FREE_MEM_ROS;
			}
		}
		else
		{
			return SUCCESS_ROS;
		}
	}
}
				


void _EraseMsgEntry_ROS
		(
			uint8_t message_index
		)
{
	gMsgTable_ROS[message_index][MSG_ID_ROS] = NULL_ID_ROS;
	gMsgTable_ROS[message_index][MSG_SIZE_ROS] = NULL_SIZE_ROS;
	gMsgTable_ROS[message_index][MSG_LOC_ROS] = NULL_LOC_ROS;
	gMsgTable_ROS[message_index][MSG_TTL_ROS] = NULL_TTL_ROS;
	gMsgTable_ROS[message_index][MSG_TARG_ROS] = NULL_TARG_ROS;
}

void _EraseDelMsgEntry_ROS
		(
			uint8_t message_index
		)
{
	gDelMsgTable_ROS[message_index][MSG_ID_ROS] = NULL_ID_ROS;
	gDelMsgTable_ROS[message_index][MSG_SIZE_ROS] = NULL_SIZE_ROS;
	gDelMsgTable_ROS[message_index][MSG_LOC_ROS] = NULL_LOC_ROS;
	gDelMsgTable_ROS[message_index][MSG_TTL_ROS] = NULL_TTL_ROS;
	gDelMsgTable_ROS[message_index][MSG_TARG_ROS] = NULL_TARG_ROS;

}
	
/***************************************************************************************************
* Name			: _IsMessageIDValid_ROS
* Type			: Internal function, input validation.
* Description	: Checks passed message ID is within maximum and minimum ranges, as defined by
*				  MAX_MSG_ID_ROS and MIN_MESSAGE_ID_ROS (declaration in messages.h). Returns 
*				  too high or too low error code if not in range, otherwise returns true.
* Notes			: None.
***************************************************************************************************/					
uint8_t _IsMessageIDValid_ROS
		(
			/* Message ID to validate */
			uint8_t message_id
		)
{
	/* Check if message ID is greater than maximum defined value */
	if(message_id > MAX_MSG_ID_ROS)
	{
		/* Message ID too high, return failure */
		return F_MSG_ID_TOO_HIGH_ROS;
	}
	/* Check if message ID is lower than minimum defined value */
	else if(message_id < MIN_MSG_ID_ROS)
	{
		/* Message ID too low, return failure */
		return F_MSG_ID_TOO_LOW_ROS;
	}
	/* No fail conditions met, message ID is within range */
	else
	{
		/* Input validation passed, return true */
		return TRUE_ROS;
	}
}
/***************************************************************************************************
* End of _IsMessageIDValid_ROS
***************************************************************************************************/

/***************************************************************************************************
* Name			: _IsMessageIDEmpty_ROS
* Type			: Internal function, input validation.
* Description	: Checks if passed message ID contains a valid message. If the message ID is
*				  invalid, it returns an error code; otherwise it returns true if the ID is empty or
*				  false if its occupied.
* Notes			: This function uses _IsMessageIDValid_ROS to check if passed message ID is valid.
***************************************************************************************************/
uint8_t _IsMessageIDEmpty_ROS
		(
			/* Message ID to check */
			uint8_t message_id
		)
{
	/* Check if passed message ID is valid, and store result in temporary container variable */
	uint8_t message_id_valid = _IsMessageIDValid_ROS(message_id);
	
	/* Check ID validation check did not return true */
	if(message_id_valid != TRUE_ROS)
	{
		/* Message ID invalid, return error code from validation check function */
		return message_id_valid;
	}
	/* Message ID is valid */
	else
	{
		/* Look up message ID's index number, and store in temporary container variable */
		uint8_t message_index = gMsgIndexArray_ROS[message_id];
		
		/* Check if the message ID's index number is a null value (defined in messages.h) */
		if(message_index == NULL_ID_ROS)
		{
			/* Message index is null, therefore ID is empty - return true */
			return TRUE_ROS;
		}
		/* Message index is not a null value */
		else
		{
			/* Message index is not null, there ID is occupied - return false */
			return FALSE_ROS;
		}
	}
}
/***************************************************************************************************
* End of _IsMessageIDEmpty_ROS
***************************************************************************************************/

/***************************************************************************************************
* Name			: _IsMessageSizeValid_ROS
* Type			: Internal function, input validation.
* Description	: Checks if passed message size is within the range defined by MAX_MESSAGE_BYTES_ROS
*				  and MIN_MESSAGE_BYTES_ROS; and returns an error code if outside this range and 
*				  true if within the range. The max/min values are defined in messages.h.
* Notes			: 
***************************************************************************************************/
uint8_t _IsMessageSizeValid_ROS
		(
			/* Message size to validate */
			uint8_t message_size
		)
{
	/* Check if message size is above maximum */
	if(message_size > MAX_MSG_BYTES_ROS)
	{
		/* Message size greater than maximum, return failure */
		return F_MSG_SIZE_TOO_LARGE_ROS;
	}
	/* Check if message size is below minimum */
	else if(message_size < MIN_MSG_BYTES_ROS)
	{
		/* Message size lower than minimum, return failure */
		return F_MSG_SIZE_TOO_LOW_ROS;
	}
	/* Message size is within range */
	else
	{
		/* Message size is within range, return true */	
		return TRUE_ROS;
	}
}
/***************************************************************************************************
* End of _IsMessageSizeValid_ROS
***************************************************************************************************/