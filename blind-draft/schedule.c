/* RataOS Task Scheduler */

/* System Parameters */
#define MAX_TASK_QUEUE_ROS			32u
#define PRIORITY_DEADLINE_SCALER	3u

/* API Control Parameters */


/* Error Return Codes */
 

uint8_t gOSTaskQueue_ROS[MAX_TASK_QUEUE_ROS];


uint8_t QueueTask_ROS
	    (
	    	uint8_t task_vector, \
	    	uint8_t 
		)
{
	if(task_vector
