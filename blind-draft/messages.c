/***************************************************************************************************
* RataOS Task Scheduler
* File 				: message.c
* Description   	: Message adminstration API definitions.
* Revision History	: Unreleased
* License			: Eclipse Public License
*					  http://www.opensource.org/licenses/eclipse-1.0.php
* Authors			: Oliver Kent
* Project Location	: http://rataos.sourceforge.net/
***************************************************************************************************/

/***************************************************************************************************
* Header Includes
***************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "messages.h"

/***************************************************************************************************
* Global Variables
***************************************************************************************************/
/* Message data storage array */
uint8_t gMsgStorArray_ROS[MAX_MSG_STOR_BYTES_ROS];
/* Message packet table */
uint8_t gMsgTOC_ROS[MAX_MSGS_ROS][MAX_MSG_ATTR_ROS];
/* Deleted message packet table */
uint8_t gMsgDTOC_ROS[MAX_DEL_MSGS_ROS][MAX_DEL_MSG_ATTR_ROS];
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

uint8_t * gMsgFileSysPtr_ROS;

uint32_t gMsgFileSysMaxBytes_ROS = 0;


bool gMsgFileSysMounted_R0S = false;

/***************************************************************************************************
* Local Function Prototypes
***************************************************************************************************/
/* Check message owner function */
uint8_t _IsMessageOwner_ROS(uint8_t);
/* Check message ID is valid function */
uint8_t _IsMessageIDValid_ROS(uint8_t);
/* Check message ID is empty function */
uint8_t _IsMessageIDEmpty_ROS(uint8_t);
/* Check message size is valid function */
uint8_t _IsMessageSizeValid_ROS(uint8_t);
/* Find a space for a new message function */
uint8_t _FindMsgSpace_ROS(uint8_t, uint8_t *, uint8_t *, bool *, uint8_t *);
/* Erase message table entry function */
void _EraseMsgEntry_ROS(uint8_t);
/* Erase delete message table entry function */
void _EraseDelMsgEntry_ROS(uint8_t);


uint8_t _WriteMessageData_ROS(uint8_t *, uint8_t *, uint8_t);


uint8_t MountMessageFileSystem_ROS
		(
			uint8_t * start_pointer, \
			uint32_t block_size, \
			uint32_t max_messages, \
			uint32_t max_deleted_messsages, \
			uint32_t * first_fail_location
		)
{
	uint32_t i;
	uint8_t * test_pointer = start_pointer;
	
	for(i = 0; i < block_size; i++)
	{

		*test_pointer = 0xAA;
		
		if(*test_pointer != 0xAA)
		{
			*first_fail_location = i;
			
			return F_MSG_FS_MOUNT_TEST_FAIL_ROS;
		}
		
		test_pointer++;
	}

	memset(start_pointer, 0xFF, block_size);

	gMsgFileSysMounted_R0S = true;
		
	gMsgFileSysPtr_ROS = start_pointer;
		
	gMsgFileSysMaxBytes_ROS = block_size;	
		
	return 0;
}
 
uint8_t ReadMessage_ROS
		(
			uint8_t message_id, \
			uint8_t num_bytes, \
			uint8_t * pointer_to_destination
		)
{
	uint8_t is_empty;
	
	is_empty = _IsMessageIDEmpty_ROS(message_id);
	
	if(is_empty == TRUE_ROS)
	{
		return F_MSG_ID_EMPTY_ROS;
	}
	else if(is_empty != FALSE_ROS)
	{
		return is_empty;
	}
	else
	{
		uint8_t message_index;
		
		message_index = gMsgIndexArray_ROS[message_id];
		
		if(num_bytes > gMsgTOC_ROS[message_index][MSG_SIZE_ROS])
		{
			return F_READ_GREATER_MSG_SIZE_ROS;
		}
		else
		{
			uint8_t * message_location = gMsgFileSysPtr_ROS;
			
			message_location += gMsgTOC_ROS[message_index][MSG_LOC_ROS];
			
			num_bytes = num_bytes == 0 ? gMsgTOC_ROS[message_index][MSG_SIZE_ROS] : num_bytes;
		
			strncpy(pointer_to_destination, message_location, num_bytes);
			
			return SUCCESS_ROS;
		}
	}
	
	return 0;
}

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
*				  stored in gMsgTOC_ROS, and a lookup entry inserted into gMsgIndexArray_ROS. 
* Notes			: 1. If this function is interrupted by any other message function, the created 
*					 message may be corrupted.
*				  2. Time to live and message targets have not been fully implemented, although they
*				     are enterted into gMsgTOC_ROS.
* DEV			: [OK] Develop a FastCreateMessage_ROS function that always puts the message ontop
*					   of the last (bypass looking for deleted locations)?
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

	if(!gMsgFileSysMounted_R0S)
	{
		return F_MSG_FS_NOT_MOUNTED_ROS;
	}
	/* Check if empty message check returned false */
	else if(id_empty == FALSE_ROS)
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
			/* Declare variable to store the write message operation result */
			uint8_t write_result;
			
			/* Store the new message index into the ID -> index lookup table */
			gMsgIndexArray_ROS[message_id] = message_index;

			/* Store the message parameteres in the message table */
			gMsgTOC_ROS[message_index][MSG_ID_ROS] = message_id;
			gMsgTOC_ROS[message_index][MSG_SIZE_ROS] = message_size;
			gMsgTOC_ROS[message_index][MSG_LOC_ROS] = message_location;
			gMsgTOC_ROS[message_index][MSG_TTL_ROS] = time_to_live;
			gMsgTOC_ROS[message_index][MSG_TARG_ROS] = target_vector;

			/**
			 * DEV: [OK] Message storage should move towards a pointer based location, so that the
			 * 			 message file system can be mount in a startup routine, to a user defined 
			 *			 location.
			 * 
			 *      [OK] Define a memory start address with a pointer, and user pointer arithmetic
			 *			 to address different memory locations?
			 **/
			 
			write_result =	_WriteMessageData_ROS
							(
								pointer_to_message, \
								(gMsgFileSysPtr_ROS + message_location), \
								message_size
							);
							
			if(write_result != SUCCESS_ROS)
			{
				return write_result;
			}		
			/* Check the is_deleted_location flag, to check if the new message is in a deleted 
			   location */
			else if(is_deleted_location)
			{
			
			
			/**
			* DEV: [OK] If deleted message space is larger than message size, bytes are wasted
			*            no longer retrievable. Need to check this, and update deleted message
			*           entry, not always delete it.
			**/
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
	
	/**
	 * DEV: [OK] Code should never reach this point. Do we need a return value?
	 **/
	 
	return 0;
}
/***************************************************************************************************
* End of CreateMessage_ROS
***************************************************************************************************/

/***************************************************************************************************
* Name			: DeleteMessage_ROS
* Type			: API function, message system
* Description	: This function deletes messages from the message ID, which is passed as the
*				  function's only arguement. The following conditions will cause the delete
*				  operation to fail:
*					- Invalid message ID
*					- Empty message ID
*					- Maximum number of deleted messages reached
*				  The function stores the contains of the message to delete into a free deleted 
*				  messages slot, then clears the entries from the main message table. The message
*				  data is not cleared.
* Notes			: Repeatedly deleting small messages in odd locations (i.e. 1 byte deleted from 
*				  locations 1, 3, 5, 7...), and creating larger messages will make inefficent usage
*				  of the memory space. The small deleted messages leave 'holes' which must be need 
*				  to be removed by defragementation. Deleting and creating messages of the same size
*				  or multiples of the same size helps to alleiveate this problem.
* DEV			: [OK] Need to setup a defrag function. Perform defrag as part of the delete to 
*					   ensure a delete always happens? Or request a defrag once number of deleted
*					   messages reaches a threshold?
***************************************************************************************************/
uint8_t DeleteMessage_ROS
		(
			/* Message ID to delete */
			uint8_t message_id
		)
{
	/* Declare input validation result container variable */
	uint8_t is_id_empty;
	
	/* Check if message ID is valid, and contains a message. Store result in container variable */
	is_id_empty = _IsMessageIDEmpty_ROS(message_id);

	if(!gMsgFileSysMounted_R0S)
	{
		return F_MSG_FS_NOT_MOUNTED_ROS;
	}
	/* Check message empty check result is true */
	else if(is_id_empty == TRUE_ROS)
	{
		/* Message ID does not contain a message, cannot delete. Return failure */
		return F_MSG_ID_EMPTY_ROS;
	}
	/* Check if message empty check result is not true or false (must be error code) */
	else if(is_id_empty != FALSE_ROS)
	{
		/* Message ID invalid, container variable contains error code to return. Return failure */
		return is_id_empty;
	}
	/* Input validation successful, begin delete operation */
	else
	{
		/* Declare loop counter varaible, and deletion index container variable */
		uint8_t i, del_index;

		/* Declare deleted slot found boolean status flag (true = slot found), intialise to false */
		bool found_del_slot = false;

		/* Begin a for loop to check each deleted message table entry for an empty slot */
		for(i = 0; i < MAX_DEL_MSGS_ROS; i++)
		{
			/* Check if deleted message ID is null value (deleted slot is free) */
			if(gMsgDTOC_ROS[i][MSG_ID_ROS] == NULL_ID_ROS)
			{
				/* Store the empty deleted message index */
				del_index = i;

				/* Set the boolean deleted slot found flag to true */
				found_del_slot = true;

				/* Break from the for loop prematurely, slot found */
				break;
			}
		}

		/* Check if slot found flag is false */
		if(!found_del_slot)
		{
			/**
			 * DEV: [OK] Raise defrag request here? 
			 **/
		
			/* Need to defrag, no deleted table space left */
			return F_MAX_DEL_MSGS_REACHED_ROS;
		}
		/* Slot found flag is true, continue delete operation */
		else
		{
			/* Retrieve message to delete's index, and store in container variable */
			uint8_t message_index = gMsgIndexArray_ROS[message_id];

			/* Remove lookup table entry for the message to delete, and set to null value */
			gMsgIndexArray_ROS[message_id] = NULL_ID_ROS;

			/* Increment the number of deleted messages by one */
			gNumDelMsg_ROS++;

			/* Increase the number of deleted bytes by the size of the message now being deleted */
			gNumDelBytes_ROS += gMsgTOC_ROS[message_index][MSG_SIZE_ROS];

			/* Copy the message to delete's parameters to the deleted message table */
			gMsgDTOC_ROS[del_index][MSG_ID_ROS] = gMsgTOC_ROS[message_index][MSG_ID_ROS];
	 		gMsgDTOC_ROS[del_index][MSG_SIZE_ROS] = gMsgTOC_ROS[message_index][MSG_SIZE_ROS];
			gMsgDTOC_ROS[del_index][MSG_LOC_ROS] = gMsgTOC_ROS[message_index][MSG_LOC_ROS];
			gMsgDTOC_ROS[del_index][MSG_TTL_ROS] = gMsgTOC_ROS[message_index][MSG_TTL_ROS];
			gMsgDTOC_ROS[del_index][MSG_TARG_ROS] = gMsgTOC_ROS[message_index][MSG_TARG_ROS];
			gMsgDTOC_ROS[del_index][MSG_OLDINDEX_ROS] = message_index;

			/* Erase the deleted message's parameters from the main message table */
			_EraseMsgEntry_ROS(message_index);

			/* Deletion operation successful, return success */
			return SUCCESS_ROS;
		}
	}
}
/***************************************************************************************************
* End of DeleteMessage_ROS
***************************************************************************************************/

/***************************************************************************************************
* Name			: _FindMsgSpace_ROS
* Type			: Internal function, message system
* Description	: This function attempts to find a space for a message of a specified size.
*				  function first looks for a deleted message space, and then looks finding a space
*				  on top of the last message. If the function is succesful, it will return 
*				  SUCCESS_ROS, and output the message location and index to the pointers passed. If
*				  slot was a deleted message, it will also set the is_deleted_location pointer 
*				  variable to true, and specify the delte message's index. If function cannot find 
*				  space for the message, or the maximium number of messages has been reached - the
*				  function will return failure with an error code.
* Notes			: In a large deleted message array, finding a free deleted message location can be
*				  instruction heavy.
* DEV			: [OK] Include input validation for message size?
***************************************************************************************************/
uint8_t _FindMsgSpace_ROS
		(
			/* All output values are invalid in function does not return SUCCESS_ROS */
			/* Size of message to find space for */
			uint8_t message_size, \
			/* Pointer to variable that will store the message space's location */
			uint8_t * output_location, \
			/* Pointer to variable that will store the message space's index */
			uint8_t * message_index, \
			/* Pointer to variable that will store a status flag to indicate whether the message
			   space is a deleted location (true = yes) */
			bool * is_deleted_location, \
			/* Pointer to variable that will store index of the deleted message the new space 
			   occupies (value only valid when is_deleted_location is true) */
			uint8_t * deleted_message_index 
		)
{
	/* Check if the maximum number of messages has been reached */
	if(gNumMsg_ROS >= MAX_MSGS_ROS)
	{
		/* Maximum number of messages reached, return failure */
		return F_MAX_MSGS_REACHED_ROS;
	}
	/* Input validation complete, continue to find message space */
	else
	{
		/* Declare space found status flag (true = space found), initialise to false */
		bool space_found = false;
	
		/* Check if the total number of deleted bytes is greater than or equal to the message size
		   (if total number of deleted bytes is lower, there can't be enough space in deleted 
		   messages */
		if(gNumDelBytes_ROS >= message_size)
		{
			/* Declare temporary loop counter variable */
			uint8_t i;
	
			/* Iterate through the entire deleted messages table */
			for(i = 0u; i < MAX_DEL_MSGS_ROS; i++)
			{
				/* Check if the deleted message entry [i] is big enough to store the message */
				if(gMsgDTOC_ROS[i][MSG_SIZE_ROS] >= message_size)
				{
					/* Deleted message entry is big enough to fit message, set the output location
					   pointer to the location of the deleted message */
					*output_location = gMsgDTOC_ROS[i][MSG_LOC_ROS];

					/* Set the deleted location status flag pointer to true */
					*is_deleted_location = true;

					/* Set the deleted message index pointer to i */
					*deleted_message_index = i;

					/* Set the new message index value to the deleted message's index */
					*message_index = gMsgDTOC_ROS[i][MSG_OLDINDEX_ROS];

					/* Set the space found flag to true */
					space_found = true;

					/* Suitable deleted message space found, break from for loop prematurely */
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
	return 0;
}
				
uint8_t _WriteMessageData_ROS
		(
			uint8_t * pointer_to_data, \
			uint8_t * pointer_to_destination, \
			uint8_t number_bytes
		)
{
	if(0)
	{
		/* Input validation */
	}
	else
	{
		

		memmove(pointer_to_destination, pointer_to_data, number_bytes);	
			
		/**
		* DEV: [OK] Add different write options here, for writing to EEPROM/SDRAM etc? 
		*       Perhaps an extension, or user defined function elsewhere. Will only use
		*		 internal flash memory for first release.
		**/
		 
		return SUCCESS_ROS;
	}
	
	return 0;	
}

void _EraseMsgEntry_ROS
		(
			uint8_t message_index
		)
{
	gMsgTOC_ROS[message_index][MSG_ID_ROS] = NULL_ID_ROS;
	gMsgTOC_ROS[message_index][MSG_SIZE_ROS] = NULL_SIZE_ROS;
	gMsgTOC_ROS[message_index][MSG_LOC_ROS] = NULL_LOC_ROS;
	gMsgTOC_ROS[message_index][MSG_TTL_ROS] = NULL_TTL_ROS;
	gMsgTOC_ROS[message_index][MSG_TARG_ROS] = NULL_TARG_ROS;
}

void _EraseDelMsgEntry_ROS
		(
			uint8_t message_index
		)
{
	gMsgDTOC_ROS[message_index][MSG_ID_ROS] = NULL_ID_ROS;
	gMsgDTOC_ROS[message_index][MSG_SIZE_ROS] = NULL_SIZE_ROS;
	gMsgDTOC_ROS[message_index][MSG_LOC_ROS] = NULL_LOC_ROS;
	gMsgDTOC_ROS[message_index][MSG_TTL_ROS] = NULL_TTL_ROS;
	gMsgDTOC_ROS[message_index][MSG_TARG_ROS] = NULL_TARG_ROS;

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
