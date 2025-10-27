/*
 * Discounted_FreeRTOS.c
 *
 *  Created on: Oct 27, 2025
 *      Author: NGUYEN BINH
 */

#include "main.h"
#include "scheduler.h"

#ifdef SCH_1

sTask SCH_tasks_G[SCH_MAX_TASK];
sTask* queue_head = NULL;
uint32_t queue_tasks_num = 0;
ERROR_REPORT Error_code_G;
ERROR_REPORT Last_error_code_G;
uint16_t Error_tick_count_G = 0;

void SCH_Init(void){
	Error_code_G = 0;

	if(!(queue_head == NULL && queue_tasks_num == 0)){ //if queue not empty, delete the queue
		sTask* temp = NULL;
		while(queue_head){
			temp = queue_head;
			queue_head = queue_head->next;
			free(temp);
		}
		queue_tasks_num = 0;
	}

	for(uint32_t i = 0; i < SCH_MAX_TASK; i++){ //initialize tasks array
		SCH_tasks_G[i].pTask = NULL;
		SCH_tasks_G[i].delay = SCH_tasks_G[i].period = SCH_tasks_G[i].RunMe = SCH_tasks_G[i].TaskID = 0;
	}

	Error_code_G = 0;
}

uint32_t SCH_Add_Task(void (* pFunction) () , uint32_t DELAY, uint32_t PERIOD){
	if(pFunction == NULL || DELAY < 0 || PERIOD < 0){ //check if invalid add
		Error_code_G = ERROR_SCH_INVALID_TASK_INIT;
		return -1;
	}

	uint32_t i = 0;
	while((i < SCH_MAX_TASK) && (SCH_tasks_G[i].pTask != NULL)){
		i++;
	}

	if(i == SCH_MAX_TASK){
		Error_code_G = ERROR_SCH_TOO_MANY_TASKS;
		return SCH_MAX_TASK;
	}

	SCH_tasks_G[i].pTask = pFunction;
	SCH_tasks_G[i].delay = DELAY;
	SCH_tasks_G[i].period = PERIOD;
	SCH_tasks_G[i].RunMe = 0;
	SCH_tasks_G[i].TaskID = i;

	if(queue_head == NULL && queue_tasks_num == 0){//if queue is empty, add one task to queue
		queue_head = (sTask*) malloc(sizeof(sTask));
		if (queue_head == NULL) return -1; // allocation failed
		*queue_head = SCH_tasks_G[i];
		queue_head->next = NULL;
	}
	else{//if not
		sTask* new_task = (sTask*) malloc(sizeof(sTask));
		if (new_task == NULL) return -1;
		*new_task = SCH_tasks_G[i];
		new_task->next = NULL;

		sTask* cursor = queue_head; //create a cursor
		sTask* prev = NULL;
		uint32_t delay_accumulate = 0;

		while (cursor != NULL && (new_task->delay > delay_accumulate + cursor->delay)) { //if cursor is not at the end of the queue and accumulated delay is still smaller than tasks delay
			delay_accumulate += cursor->delay; //accumulate delay
			prev = cursor;
			cursor = cursor->next; // keep moving to next
		}

		new_task->delay -= delay_accumulate; //calculate relative delay

		if (prev == NULL) { //if empty queue, insert at head
			new_task->next = queue_head;
			if (queue_head != NULL) {
				queue_head->delay -= new_task->delay;
			}
			queue_head = new_task;
		}
		else { // if not
			new_task->next = cursor; //insert in between node
			prev->next = new_task;

			if (cursor != NULL) { //recalculate the next task delay
				cursor->delay -= new_task->delay;
			}
		}
	}
	queue_tasks_num++;

	return i;
}

void SCH_Update(void){
	if (queue_head == NULL) return; //if empty queue return

	if(queue_head->delay > 0){ //decrement head
		queue_head->delay--;
	}

	sTask* cursor = queue_head;
	while(cursor != NULL && cursor->delay == 0){
		cursor->RunMe++;
		cursor = cursor->next;
	}
}

void SCH_Dispatch_Tasks(void){
	while(queue_head != NULL && queue_head->RunMe > 0){ //Loop through all task that's had runme flag on from head

		if(queue_head->pTask != NULL){
			(*queue_head->pTask)(); //run the tasks
			queue_head->RunMe = 0;

			if(queue_head->period > 0){ //if it is periodically
				SCH_RescheduleTask();
			}
			else{
				SCH_Delete_Task(queue_head->TaskID);
			}

		}

	}

	SCH_Report_Status(); //output status
	SCH_Go_To_Sleep(); //sleep
}

uint8_t SCH_Delete_Task(uint32_t TASK_INDEX){
	if(TASK_INDEX < 0 || TASK_INDEX >= SCH_MAX_TASK || SCH_tasks_G[TASK_INDEX].pTask == NULL){ //invalid index
		Error_code_G = ERROR_SCH_CANNOT_DELETE_TASK;
		return -1;
	}
	//delete from tasks array
	SCH_tasks_G[TASK_INDEX].pTask = NULL;
	SCH_tasks_G[TASK_INDEX].delay = 0;
	SCH_tasks_G[TASK_INDEX].period = 0;
	SCH_tasks_G[TASK_INDEX].RunMe = 0;
	SCH_tasks_G[TASK_INDEX].TaskID = 0;

	//delete from queue
	sTask* cursor = queue_head;
	sTask* prev = NULL;
	while(cursor != NULL){
		if(cursor->TaskID == TASK_INDEX){
			break;
		}
		prev = cursor;
		cursor = cursor->next;
	}
	if(cursor != NULL){
		if(queue_head->TaskID == cursor->TaskID){ //delete head
			sTask* temp = queue_head;
			queue_head = queue_head->next;
			if(queue_head != NULL){
				queue_head->delay += temp->delay;
			}
			free(temp);
		}
		else{
			prev->next = cursor->next;
			if (cursor->next != NULL){
				cursor->next->delay += cursor->delay;
			}
			free(cursor);
		}
		queue_tasks_num--;
	}

	return TASK_INDEX;
}

void SCH_RescheduleTask(void){
	if (queue_head == NULL) return;

	sTask* temp = queue_head;
	sTask* cursor = queue_head->next;
	sTask* prev = NULL;
	uint32_t delay_accumulate = 0;

	while (cursor != NULL && (queue_head->period > delay_accumulate + cursor->delay)) { //if cursor is not at the end of the queue and accumulated delay is still smaller than delay of task that is being rescheduled
		delay_accumulate += cursor->delay; //accumulate delay
		prev = cursor;
		cursor = cursor->next; // keep moving to next
	}

	queue_head = queue_head->next;
	temp->delay = temp->period - delay_accumulate;

	if (prev == NULL) {
		temp->next = queue_head;
		if (queue_head != NULL){
			queue_head->delay -= temp->delay;
		}
		queue_head = temp;
	}
	else {
		// Insert after 'prev'
		temp->next = cursor;
		prev->next = temp;
		if (cursor != NULL){
			cursor->delay -= temp->delay;
		}
	}
}

void SCH_Go_To_Sleep(void){

}

void Error_Output(){
	char c[64];
	switch(Error_code_G){
	case ERROR_SCH_NORMAL:
		return;
		break;
	case ERROR_SCH_TOO_MANY_TASKS:
		sprintf(c, "ERROR_SCH_TOO_MANY_TASKS");
		break;
	case ERROR_SCH_INVALID_TASK_INIT:
		sprintf(c, "ERROR_SCH_INVALID_TASK_INIT");
		break;
	case ERROR_SCH_CANNOT_DELETE_TASK:
		sprintf(c, "ERROR_SCH_CANNOT_DELETE_TASK");
		break;
	case ERROR_SCH_WAITING_FOR_SLAVE_TO_ACK:
		sprintf(c, "ERROR_SCH_WAITING_FOR_SLAVE_TO_ACK");
		break;
	case ERROR_SCH_WAITING_FOR_START_COMMAND_FROM_MASTER:
		sprintf(c, "ERROR_SCH_WAITING_FOR_START_COMMAND_FROM_MASTER");
		break;
	case ERROR_SCH_ONE_OR_MORE_SLAVES_DID_NOT_START:
		sprintf(c, "ERROR_SCH_ONE_OR_MORE_SLAVES_DID_NOT_START");
		break;
	case ERROR_SCH_LOST_SLAVE:
		sprintf(c, "ERROR_SCH_LOST_SLAVE");
		break;
	case ERROR_SCH_CAN_BUS_ERROR:
		sprintf(c, "ERROR_SCH_CAN_BUS_ERROR");
		break;
	case ERROR_I2C_WRITE_BYTE_AT24C64:
		sprintf(c, "ERROR_I2C_WRITE_BYTE_AT24C64");
		break;
	default:
		break;
	}
	UART_Print(c, strlen(c));
}

void SCH_Report_Status(void){
#ifdef SCH_REPORT_ERRORS
	if(Error_code_G != Last_error_code_G){
		//Error_port = 255 − Error_code_G;
		Last_error_code_G = Error_code_G;
		if(Error_code_G != 0){
			Error_Output();
			Error_tick_count_G = 60000;
		}
		else{
			Error_tick_count_G = 0;
		}
	}
	else{
		if(Error_tick_count_G != 0){
			if(--Error_tick_count_G == 0) {
				Error_code_G = 0;
			}
		}
	}
#endif
}
#endif

