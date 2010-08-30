/*******************************************************************************
* RataOS Task Scheduler
* File 				: debug.c
* Description   	: Provides a debugging interface for the OS.
* Revision History	: Unreleased
* License			: Eclipse Public License
*					  http://www.opensource.org/licenses/eclipse-1.0.php
* Author			: Oliver Kent
* Project Location	: http://rataos.sourceforge.net/
*******************************************************************************/

					   


#define ENABLE_DEBUG_ROS		TRUE





#ifdef ENABLE_DEBUG_ROS
#if	(ENABLE_DEBUG_ROS)

/* Define array of pointers to debug message definitions */

uint8_t * gDebugCodeMessage[MAX_DEBUG_MESSAGES_ROS];




#endif
