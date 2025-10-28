/*
 * Discounted_FreeRTOS.h
 *
 *  Created on: Oct 27, 2025
 *      Author: NGUYEN BINH
 */

#ifndef INC_SCHEDULER_H_
#define INC_SCHEDULER_H_

#define SCH_MAX_TASK 40
#define NO_TASK_ID 0

typedef enum{
	ERROR_SCH_NORMAL = 0,
	ERROR_SCH_TOO_MANY_TASKS,
	ERROR_SCH_QUEUE_INITIALIZATION_FAILED,
	ERROR_SCH_INVALID_TASK_INIT,
	ERROR_SCH_CANNOT_DELETE_TASK,
	ERROR_SCH_WAITING_FOR_SLAVE_TO_ACK,
	ERROR_SCH_WAITING_FOR_START_COMMAND_FROM_MASTER,
	ERROR_SCH_ONE_OR_MORE_SLAVES_DID_NOT_START,
	ERROR_SCH_LOST_SLAVE,
	ERROR_SCH_CAN_BUS_ERROR,
	ERROR_I2C_WRITE_BYTE_AT24C64
}ERROR_REPORT;

typedef struct sTask{
	void (*pTask)();
	uint32_t delay;
	uint32_t period;
	uint8_t RunMe;
	uint32_t TaskID;
#ifdef SCH_1
	struct sTask* next;
#endif
}sTask;


extern sTask SCH_tasks_G[SCH_MAX_TASK];
#ifdef SCH_1
	extern sTask* queue_head;
	extern uint32_t queue_tasks_num;
#endif
extern ERROR_REPORT Error_code_G;
extern ERROR_REPORT Last_error_code_G;
extern uint16_t Error_tick_count_G;

void SCH_Init(void);
void SCH_Update(void);
uint32_t SCH_Add_Task(void (* pFunction) () , uint32_t DELAY, uint32_t PERIOD);
void SCH_Dispatch_Tasks(void);
uint8_t SCH_Delete_Task(uint32_t TASK_INDEX);
void SCH_Go_To_Sleep(void);
void SCH_Report_Status(void);
#ifdef SCH_1
void SCH_RescheduleTask(void);
#endif
#endif /* INC_SCHEDULER_H_ */
