/*******************************************************************************
* RataOS Task Scheduler
* File 				: tasks.c
* Description   	: Task adminstration API definitions.
* Revision History	: Unreleased
* License			: Eclipse Public License
*					  http://www.opensource.org/licenses/eclipse-1.0.php
* Author			: Oliver Kent
* Project Location	: http://rataos.sourceforge.net/
*******************************************************************************/

#include <string.h>
#include <type.h>

/* System Parameters */
#define MAX_TASKS_ROS					16u
#define MAX_TASK_VECTOR_ROS				250u
#define MIN_TASK_VECTOR_ROS				5u
#define MAX_TASK_INFO_ROS				10u
#define MIN_TASK_INFO_ROS				3u
#define MIN_TASK_TIMEOUT_ROS			5u
#define MAX_TASK_TIMEOUT_ROS			200u
#define INTIAL_NUMBER_TASKS_ROS			0u

/* API Control Parameters */
#define TASK_PROTECTION_ENABLE_ROS		0x05
#define TASK_PROTECTION_DISABLE_ROS		0x06

/* Misc */
#define NULL_POINTER_ROS				0u
#define NULL_CHARACTER_ROS				0u
#define NULL_TASK_ROS					0u

/* Return Value Codes */
#define SUCCESS_ROS						0x01
#define TRUE_ROS						0x02
#define FALSE_ROS						0x03
#define F_TASK_VECTOR_OCCUPIED_ROS		0x05
#define F_TASK_VECTOR_EMPTY_ROS			0x06
#define F_TASK_INFO_TOO_SMALL_ROS		0x07
#define F_TASK_TIMEOUT_TOO_LOW_ROS		0x08
#define F_TASK_TIMEOUT_TOO_HIGH_ROS		0x09
#define F_TASK_PROTECTED_ROS			0x0A
#define F_TASK_VECTOR_TOO_HIGH			0x0B
#define F_TASK_VECTOR_TOO_LOW			0x0C

/* Import global operating system status */
extern uint8_t gOperatingSystemStatus_ROS;

/* Global variable to contain current number of tasks */
uint8_t gNumberGlobalTasks = INTIAL_NUMBER_TASKS_ROS;

/* Array to hold task vector ID lookup table */
uint8_t gTaskVectorLookupArray_ROS[MAX_TASK_VECTOR_ROS];

/* Array to hold pointers to task functions and their task vector */
void *gTaskPointerArray_RS[MAX_TASKS_ROS];

/* Array to hold task vector priorities */
uint8_t gTaskPriorityArray_ROS[MAX_TASKS_ROS];

/* Array to hold task timeout durations */
uint32_t gTaskTimeoutArray_ROS[MAX_TASKS_ROS];

/* Array to hold task descriptions */
uint8_t gTaskInfoArray_ROS[MAX_TASKS_ROS][MAX_TASK_INFO_ROS];

/* Array to hold task sleep status */
uint8_t gTaskSleepStatusArray_ROS[MAX_TASKS_ROS];

/* Array to hold task protected status */
bool gTaskProtectionArray_ROS[MAX_TASKS_ROS];

/* Next free task ID location */
uint8_t gTaskIDStack_ROS;

/*******************************************************************************
* Name			: CreateTask_ROS
* Description	: Creates a new task entry, allowing it to be included in the
*				  scheduler. Task is only created all inputs are valid.
* Notes			: None.
*******************************************************************************/
uint8_t CreateTask_ROS
	 	( 
	 		/* Vector to create task entry in */
	 		uint8_t task_vector, \
	 		/* Created task priority level */
	 		uint8_t task_priority, \
	 		/* Created task timeout duration, in clock ticks */
	 		uint32_t task_timeout, \
	 		/* Created task sleep status */
	 		bool task_sleep_enable \
			/* Pointer to created task description string */
	 		uint8_t * task_description, \
	 		/* Pointer to task function */
	 		void (*task_pointer)(void)
	 	)
{	
	/* Declare input validation check container variables */
	uint8_t is_task_empty, is_timeout_valid;
	
	/* Check if task vector is empty */
	is_task_empty = _IsTaskVectorEmpty_ROS(task_vector);
	
	/* Check if timeout value is valid */
	is_timeout_valid = _IsTaskTimeoutValid_ROS(task_timeout);
	
	/* Check if task empty check returned false */
	if(is_task_empty == FALSE_ROS)
	{
		/* Task occupied, return task vector occupied failure */
		return F_TASK_VECTOR_OCCUPIED_ROS;	
	}
	/* Check if task empty check return invalid task */
	else if(is_task_empty != TRUE_ROS)
	{
		/* Task vector invalid, return error code */
		return is_task_empty;
	}	
	/* Check if task timeout duration is invalid */
	else if(is_timeout_valid != TRUE_ROS)
	{
		/* Task timeout invalid, return error code */
		return is_timeout_valid;
	}
	/* Input validation successful, proceed to create task */
	else
	{
		/* Define task description length container variable, loop variable and
		   task id variable */
		uint8_t description_length = 0x00, i, task_id = gTaskIDStack_ROS;

		/* Store task id in vector lookup table */
		gTaskVectorLookupArray_ROS[task_vector] = task_id;

		/* Loop through all characters in task description, to calculate actual
		   length of description */
		for(i = 0x00; i != MAX_TASK_INFO_ROS; i++)
		{
			/* Check if location is a printable character */
			if(!iscntrl(task_description[i])
			{	
				/* Location is a printable character, increment description
				   length counter */
				description_length++;
			}
			/* Location is a control character */
			else
			{
				/* Non-printable character found, break for loop */
				break;
			}
		}

		/* Check if task description is meets minimum length */
		if(description_length < MIN_TASK_INFO_ROS)
		{
			/* Task description too short, return failure */
			return F_TASK_INFO_TOO_SMALL_ROS;
		}
		/* Task description meets requirements */
		else
		{
			/* Store task pointer in task array */
			gTaskPointerArray_ROS[task_id] = task_pointer;

			/* Store task priority in task priority array */
			gTaskPriorityArray_ROS[task_id] = task_priority;

			/* Store task timeout length (in tick cycles) */
			gTaskTimeoutArray_ROS[task_id] = task_timeout;
			
			/* Store task description in task info array */
			strncpy(gTaskInfoArray_ROS[task_id], task_description, \
			                           description_length);

			/* Store task sleep enable status in sleep status array */
			gTaskSleepStatusArray_ROS[task_id] = task_sleep_enable;

			/* Set task protection to disabled (default behaviour) */
			gTaskProtectionArray_ROS[task_id] = TASK_PROTECTION_DISABLE_ROS;
			
			/* Task creation complete, return success */
			return SUCCESS_ROS;
		}
	}
}
/*******************************************************************************
* End of CreateTask_ROS
*******************************************************************************/

/*******************************************************************************
* Name			: DestroyTask_ROS
* Description	: Destroys the task entry at the specified task vector. If the
*				  task vector was already empty, the function returns false.
* Notes			: Task entry is destroyed, but function definition remains.
*******************************************************************************/
uint8_t DestroyTask_ROS
		(
			/* Vector of task to destroy */
			uint8_t task_vector
		)
{
	/* Declare local variables to store validation results */
	uint8_t is_task_unprotected;
	
	/* Check if task is unprotected, and store result */
	is_task_unprotected = _IsTaskUnprotected_ROS(task_vector);
	
	/* Check if task is protected */
	if(is_task_unprotected == TRUE_ROS)
	{
		/* Task is protected, return failure */
		return F_TASK_PROTECTED_ROS;
	}
	/* Proceed to destroy task */
	else if (is_task_unprotected != FALSE_ROS)
	{
		/* Task empty or task vector invalid, return failure */
		return is_task_unprotected;
	}
	/* Input validation successful, proceed to delete task */
	else
	{
		/* Declare loop counter and task id variables */
		uint8_t i, task_id = gTaskVectorLookupArray_ROS(task_vector);
		
		/* Delete task vector lookup table ID entry */
		gTaskVectorLookupArray_ROS(task_vector) = NULL_TASK_ROS;
		
		/* Delete task pointer */
		gTaskPointerArray_ROS[task_id];
		
		/* Delete task priority */
		gTaskPriority_ROS[task_id];

		/* Delete task timeout length */
		gTaskTimeoutArray_ROS[task_id];

		/* Delete task sleep status */
		gTaskSleepStatusArray_ROS[task_id] = 
		
		/* Delete task description */
		for(i = 0u; i != MAX_TASK_INFO_ROS; i++)
		{
			/* Replace task description character with null */
			gTaskInfoArray_ROS[task_id][i] = NULL_CHARACTER_ROS;
		}

		/* Task destruction complete, return success */
		return SUCCESS_ROS;
	}
}
/*******************************************************************************
* End of DestroyTask_ROS
*******************************************************************************/

/*******************************************************************************
* Name			: ProtectTask_ROS
* Description	: Enables and disables task protection. Protected tasks cannot
*				  be deleted or configured in any way.
* Notes			: This function can only execute when the operating system is
*				  not running i.e. in a boot sequence, or an OS execption.
*******************************************************************************/
uint8_t ProtectTask_ROS
		(
			/* Vector of task to protect */
			uint8_t task_vector, \
			/* Enable or disable task protection (true = enable) */
			bool enable_protection
		)
{
	/* Check if task vector is empty, and store result in container variable */
	uint8_t is_task_empty = _IsTaskVectorEmpty_ROS(task_vector);

	/* Check if requested task vector exceeds maximum */
	if(is_task_empty == TRUE_ROS)
	{
		/* Task vector is empty, return failure */
		return F_TASK_VECTOR_EMPTY_ROS;
	}
	/* Check if task vector is invalid */
	else if(is_task_empty != FALSE_ROS)
	{
		/* Task vector invalid, return failure */
		return is_task_empty;
	}
	/* Check if operating system is running */
	else if(gOperatingSystemStatus_ROS == OS_RUNNING_ROS)
	{
		/* Cannot change task protection is OS is running, return failure */
		return F_OS_RUNNING_ROS;
	}
	/* Input validation successful, check if task enable requested */
	else if(enable_protection)
	{
		/* Enable task protection for this task vector */
		gTaskProtectionArray_ROS = TASK_PROTECTION_ENABLE_ROS;
		
		/* Task protection configuration complete, return success */
		return SUCCESS_ROS;
	}				
	/* Task protection disabled requested */
	else
	{
		/* Disable task protection for this task vector */
		gTaskProtectionArray_ROS = TASK_PROTECTION_DISABLE_ROS;
		
		/* Task protection configuration complete, return success */
		return SUCCESS_ROS;
	}
}
/*******************************************************************************
* End of ProtectTask_ROS
*******************************************************************************/

/*******************************************************************************
* Name			: ControlSleepTask_ROS
* Description	: Enables or disables the task's sleep status. This function can
*				  can only control unprotected tasks. A sleeping task will
*				  remain in a task schedule, but will not execute.
* Notes			: None.
*******************************************************************************/
uint8_t ControlSleepTask_ROS
		(
			/* Vector of task to control sleep status */
			uint8_t task_vector, \
			/* Control task sleep status (true = enable) */
			bool task_sleep_enable,		
		)
{
	/* Check if task is protected, and store result in container variable */
	uint8_t is_task_unprotected = _IsTaskUnprotected_ROS(task_vector);
	
	/* Check if task is protected */
	if(is_task_unprotected = FALSE_ROS)
	{
		/* Task is protected, return failure */
		return F_TASK_PROTECTED_ROS;
	}
	/* Check if task vector is valid */
	if(is_task_unprotected != TRUE_ROS)
	{
		/* Task vector invalid, return failure */
		return is_task_unprotected;
	}
	/* Input validation succesful, check if task sleep requested */
	else if(task_sleep_enable)
	{
		/* Enable task sleep for this task vector */
		gTaskSleepStatusArray_ROS[task_vector] = TASK_SLEEP_ENABLE_ROS;
		
		/* Sleep control complete, return success */
		return SUCCESS_ROS;
	}
	/* Task sleep disable requested */
	else
	{
		/* Disable task sleep for this task vector */
		gTaskSleepStqatusArray_ROS[task_vector] = TASK_SLEEP_DISABLE_ROS;
		
		/* Sleep control complete, return success */
		return SUCCESS_ROS;
	}
}
/*******************************************************************************
* End of ControlSleepTask_ROS
*******************************************************************************/

/*******************************************************************************
* Name			: _IsTaskVectorValid_ROS
* Description	: Checks if the specified task vector is valid. Returns true for
*				  valid vectors, and an error code for invalid vectors.
* Notes			: None.
*******************************************************************************/
uint8_t _IsTaskVectorValid_ROS
	 	(
	 		/* Task vector to check */
	 		uint8_t task_vector
		)
{
	/* Check if requested task vector exceeds maximum */
	if(task_vector > MAX_TASKS_ROS)
	{
		/* Task vector exceeds maximum, return vector too high */
		return F_TASK_VECTOR_TOO_HIGH;
	}
	/* Check if task vector is above minimum (reserved tasks) */
	else if(task_vector < MIN_TASK_VECTOR_ROS)
	{
		/* Task vector reserved, return vector too low */
		return F_TASK_VECTOR_TOO_LOW;
	}
	/* Task vector is valid */
	else
	{
		/* Task vector valid, return true */
		return TRUE_ROS;
	}
}
/*******************************************************************************
* End of _IsTaskVectorValid_ROS
*******************************************************************************/

/*******************************************************************************
* Name			: _IsTaskVectorEmpty_ROS
* Description	: Checks whether task vector is valid, and whether the task is
*				  vector contains a task or not. Returns true if task vector is
*				  empty, false if it is occupied and an error code for an
*				  invalid task vector.
* Notes			: None.
*******************************************************************************/
uint8_t _IsTaskVectorEmpty_ROS
		(
			/* Task vector to check */
			uint8_t task_vector
		)
{
	/* Check if task vector is valid, and store result in local variable */
	uint8_t is_task_vector_valid = _IsTaskVectorValid_ROS(task_vector);
	
	/* Task vector is valid */
	if(is_task_vector_valid == TRUE_ROS)
	{
		/* Check if task is empty */
		if(gTaskVectorLookupArray_ROS[task_vector] == NULL_TASK_ROS)
		{
			/* Task vector is empty, return true */
			return TRUE_ROS;
		}
		/* Task vector is occupied */
		else
		{
			/* Task vector not empty, return false */
			return FALSE_ROS;
		}	
	}
	/* Task vector is invalid */
	else
	{
		/* Return invalid task error code */
		return is_task_vector_valid;
	}
}
/*******************************************************************************
* End of _IsTaskVectorEmpty_ROS
*******************************************************************************/

/*******************************************************************************
* Name			: _IsTaskTimeoutValid_ROS
* Description	: Checks if the passed timeout value is within valid range. 
*				  Returns true if value is valid, and an error code if invalid.
* Notes			: None.
*******************************************************************************/
uint8_t _IsTaskTimeoutValid_ROS
		(
			/* Timeout value to check */
			uint8_t timeout
		)
{
	/* Check if timeout value is greater than the system maximum */	
	if(timeout > MAX_TASK_TIMEOUT_ROS)
	{
		/* Timeout value too high, return failure */
		return F_TASK_TIMEOUT_TOO_HIGH_ROS
	}
	/* Check if timeout value is lower than the system minimum */
	else if(timeout < MIN_TASK_TIMEOUT_ROS)
	{
		/* Timeout value too low, return failure */
		return F_TASK_TIMEOUT_TOO_LOW_ROS;		
	}
	/* Timeout value is within range */
	else
	{
		/* Timeout value is valid, return true */
		return TRUE_ROS;
	}
}
/*******************************************************************************
* End of _IsTimeoutValid_ROS
*******************************************************************************/

/*******************************************************************************
* Name			: _IsTaskUnprotected_ROS
* Description	: Checks if task vector first valid, then empty, then finally if
*				  the task is unprotected. Returns true if the task is 
*				  unprotected, false if it is protected and an error code if the
*				  task vector is empty or invalid.
* Notes			: None.
*******************************************************************************/
uint8_t _IsTaskUnprotected_ROS
		(
			/* Task vector to check */
			uint8_t task_vector
		)
{
	/* Check if task vector is empty, and store result in container variable */
	uint8_t is_task_empty = _IsTaskVectorEmpty_ROS(task_vector);
	
	/* Check if task vector is empty */
	if(is_task_empty == TRUE_ROS)
	{
		/* Task vector is empty, return failure */
		return F_TASK_VECTOR_EMPTY_ROS;
	}
	/* Check if task vector is invalid */
	else if (is_task_empty != FALSE_ROS)
	{
		/* Task vector is invalid, return failure */
		return is_task_empty;
	}
	/* Task vector is valid */
	else 
	{
		/* Store task vector id in container variable */
		uint8_t task_id = gTaskVectorLookupArray_ROS[task_vector];
		
		/* Check if task is protected */
		if(gTaskProtectionArray_ROS[task_id])
		{
			/* Task is protected, return false */
			return FALSE_ROS;
		}
		/* Task is unprotected */
		else
		{
			/* Task is unprotected, return true */
			return TRUE_ROS;
		}
	}
}
/*******************************************************************************
* End of _IsTaskProtected
*******************************************************************************/

	
