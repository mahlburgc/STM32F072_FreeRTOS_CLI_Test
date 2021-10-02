/*
 * cliTask.h
 *
 *  Created on: Sep 30, 2021
 *      Author: Christian
 */

#ifndef INC_CLITASK_H_
#define INC_CLITASK_H_

void StartCliTask(void *argument);
#define MAX_CLI_BUFFER_SIZE 100


typedef struct
{
	unsigned char buffer[MAX_CLI_BUFFER_SIZE];
} CliBuffer_t;

#endif /* INC_CLITASK_H_ */
