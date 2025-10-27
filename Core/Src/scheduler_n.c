/*
 * Discounted_FreeRTOS.c
 *
 *  Created on: Oct 27, 2025
 *      Author: NGUYEN BINH
 */

#include "main.h"
#include "scheduler.h"

#ifndef SCH_1

sTask SCH_tasks_G[SCH_MAX_TASK];
ERROR_REPORT Error_code_G;
ERROR_REPORT Last_error_code_G;
uint16_t Error_tick_count_G = 0;

void SCH_Init(void){
	for(uint32_t i = 0; i < SCH_MAX_TASK; i++){
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
	while((SCH_tasks_G[i].pTask != NULL) && (i < SCH_MAX_TASK)){
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

	return i;
}

void SCH_Update(void){
	for(uint32_t i = 0; i < SCH_MAX_TASK; i++){//loop through all tasks
		if(SCH_tasks_G[i].pTask != NULL){//if task is available
			if(SCH_tasks_G[i].delay <= 0){//and is ready to run
				SCH_tasks_G[i].RunMe++;//increment RunMe
				if(SCH_tasks_G[i].period > 0){//if run periodically and not a oneshot
					SCH_tasks_G[i].delay = SCH_tasks_G[i].period; //update delay back to period
				}
			}
			else{
				SCH_tasks_G[i].delay--;
			}
		}
	}
}

void SCH_Dispatch_Tasks(void){
	for(uint32_t i = 0; i < SCH_MAX_TASK; i++){//loop through all tasks
		if(SCH_tasks_G[i].RunMe > 0 && SCH_tasks_G[i].pTask != NULL){ //if task is available and is ready to run
			(*SCH_tasks_G[i].pTask)(); //run the task
			SCH_tasks_G[i].RunMe = 0;	//reset RunMe flag
			if(SCH_tasks_G[i].period == 0){ //if it is a one shot, delete from tasks array
				SCH_Delete_Task(i);
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

	SCH_tasks_G[TASK_INDEX].pTask = NULL;
	SCH_tasks_G[TASK_INDEX].delay = 0;
	SCH_tasks_G[TASK_INDEX].period = 0;
	SCH_tasks_G[TASK_INDEX].RunMe = 0;

	return TASK_INDEX;
}
void SCH_Go_To_Sleep(void){

}

void SCH_Report_Status(void){
#ifdef SCH_REPORT_ERRORS
	if(Error_code_G != Last_error_code_G){
		Error_port = 255 − Error_code_G;
		Last_error_code_G = Error_code_G;
		if(Error_code_G != 0){
			Error_tick_count_G = 60000;
		}
		else{
			Error_tick_count_G = 0;
		}
	}
	else{
		if(Error_tick_count_G != 0){
			if(--Error_tick_count_G <= 0) {
				Error_port = 0;
			}
		}
	}
#endif
}
#endif

