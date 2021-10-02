 /*
 * cliTask.c
 *
 *  Created on: Sep 30, 2021
 *      Author: Christian
 */

#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_uart.h"
#include "main.h"
#include "cmsis_os.h"
#include "string.h"
#include "cliTask.h"
#include "task.h"
#include "FreeRTOS_CLI.h"

extern UART_HandleTypeDef huart1;
extern osMessageQueueId_t cliQueueHandle;

#define MAX_OUTPUT_LENGTH   100

static uint8_t rxBufferReady = 0;
static CliBuffer_t input = { 0 };

static const int8_t * const pcWelcomeMessage =
"\
**************************************************************\r\n\
* Welcome to the Command Line Interface.                     *\r\n\
* Type Help to view a list of registered commands.           *\r\n\
**************************************************************\r\n\r\n";

char clearScreen[] = "\033[2J"; /* VT100 escape sequence to clear the screen */
char resetCursor[] = "\033[H";  /* VT100 escape sequence to set cursor to upper left corner */

BaseType_t cliToggleLed(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t cliTurnOnLed(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t cliTurnOffLed(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t cliClearScreen(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);


/* command definitions */
static const CLI_Command_Definition_t toggleLedCmd =
{
    "toggleLed",
	"toggleLed:\r\n toggles the blue led\r\n\r\n",
	cliToggleLed,
    0
};

static const CLI_Command_Definition_t turnOnLedCmd =
{
    "turnOnLed",
	"turnOnLed <led-color>:\r\n turn on led <blue>, <red>, <orange> or <green>\r\n\r\n",
	cliTurnOnLed,
    1
};

static const CLI_Command_Definition_t turnOffLedCmd =
{
    "turnOffLed",
	"turnOffLed <led-color>:\r\n turn off led <blue>, <red>, <orange> or <green>\r\n\r\n",
	cliTurnOffLed,
    1
};

static const CLI_Command_Definition_t clearScreenCmd =
{
    "clear",
	"clear:\r\n clear the screen if terminal supports VT100\r\n\r\n",
	cliClearScreen,
    0
};


void StartCliTask(void *argument)
{

	BaseType_t xMoreDataToFollow;

	const uint32_t UART_TIMEOUT = 500;

	/* The input and output buffers are declared static to keep them off the stack. */
	CliBuffer_t localInput = { 0 };
	CliBuffer_t output = { 0 };
	uint8_t rxChar = 0;


	FreeRTOS_CLIRegisterCommand(&toggleLedCmd);
	FreeRTOS_CLIRegisterCommand(&turnOnLedCmd);
	FreeRTOS_CLIRegisterCommand(&turnOffLedCmd);
	FreeRTOS_CLIRegisterCommand(&clearScreenCmd);


	HAL_UART_Transmit(&huart1, clearScreen, sizeof(clearScreen), UART_TIMEOUT);
	HAL_UART_Transmit(&huart1, resetCursor, sizeof(resetCursor), UART_TIMEOUT);
	HAL_UART_Transmit(&huart1, pcWelcomeMessage, strlen(pcWelcomeMessage), UART_TIMEOUT);

	HAL_UART_Receive_IT(&huart1, &rxChar, sizeof(rxChar)); /* reveid first byte because its an failed byte */

	while (1)
	{
		while(!rxBufferReady)
		{
			osDelay(1);
		}

		do
		{
			memcpy(localInput.buffer, input.buffer, MAX_CLI_BUFFER_SIZE);
			memset(input.buffer, 0x00, MAX_CLI_BUFFER_SIZE);

			HAL_UART_Transmit(&huart1, "\r\n", strlen( "\r\n" ), 100);

			xMoreDataToFollow = FreeRTOS_CLIProcessCommand
						  (
						      localInput.buffer,   /* The command string.*/
							  output.buffer,  /* The output buffer. */
							  MAX_CLI_BUFFER_SIZE/* The size of the output buffer. */
						  );

			/* TODO possible memory leak if buffer is not null terminated */
			HAL_UART_Transmit(&huart1, output.buffer, strlen(output.buffer), 100);

		} while( xMoreDataToFollow != pdFALSE );
		rxBufferReady = 0;
	}
}

/* user defined command example */
BaseType_t cliToggleLed(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
    (void) xWriteBufferLen;

	HAL_GPIO_TogglePin(LD6_GPIO_Port, LD6_Pin);
	sprintf(pcWriteBuffer, "  LED was toggled\r\n\r\n");

    return pdFALSE;
}

BaseType_t cliTurnOnLed(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	int8_t *pcParameter1;
	BaseType_t xParameter1StringLength, xResult;

	/* Obtain the name of the source file, and the length of its name, from
	    the command string. The name of the source file is the first parameter. */
	    pcParameter1 = FreeRTOS_CLIGetParameter
	                        (
	                          /* The command string itself. */
	                          pcCommandString,
	                          /* Return the first parameter. */
	                          1,
	                          /* Store the parameter string length. */
	                          &xParameter1StringLength
	                        );

	if (strncmp(pcParameter1,"blue",xParameter1StringLength) == 0)
	{
		HAL_GPIO_WritePin(LD6_GPIO_Port, LD6_Pin, GPIO_PIN_SET);
	}
	else if (strncmp(pcParameter1,"orange",xParameter1StringLength) == 0)
	{
		HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_SET);
	}
	else if (strncmp(pcParameter1,"red",xParameter1StringLength) == 0)
	{
		HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
	}
	else if (strncmp(pcParameter1,"green",xParameter1StringLength) == 0)
	{
		HAL_GPIO_WritePin(LD5_GPIO_Port, LD5_Pin, GPIO_PIN_SET);
	}

    sprintf(pcWriteBuffer, "  %s LED was turned on\r\n\r\n", pcParameter1);

    return pdFALSE;
}

BaseType_t cliTurnOffLed(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	int8_t *pcParameter1;
	BaseType_t xParameter1StringLength, xResult;

	/* Obtain the name of the source file, and the length of its name, from
	    the command string. The name of the source file is the first parameter. */
	    pcParameter1 = FreeRTOS_CLIGetParameter
	                        (
	                          /* The command string itself. */
	                          pcCommandString,
	                          /* Return the first parameter. */
	                          1,
	                          /* Store the parameter string length. */
	                          &xParameter1StringLength
	                        );

	if (strncmp(pcParameter1,"blue",xParameter1StringLength) == 0)
	{
		HAL_GPIO_WritePin(LD6_GPIO_Port, LD6_Pin, GPIO_PIN_RESET);
	}
	else if (strncmp(pcParameter1,"orange",xParameter1StringLength) == 0)
	{
		HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_RESET);
	}
	else if (strncmp(pcParameter1,"red",xParameter1StringLength) == 0)
	{
		HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
	}
	else if (strncmp(pcParameter1,"green",xParameter1StringLength) == 0)
	{
		HAL_GPIO_WritePin(LD5_GPIO_Port, LD5_Pin, GPIO_PIN_RESET);
	}

    sprintf(pcWriteBuffer, "  %s LED was turned off\r\n\r\n", pcParameter1);

    return pdFALSE;
}

BaseType_t cliClearScreen(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
    (void) xWriteBufferLen;

	sprintf(pcWriteBuffer, "%s %s", clearScreen, resetCursor);

    return pdFALSE;
}

/* uart rx callback */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	uint8_t rxChar = 0;
	static uint8_t inputIndex = 0;

	rxChar = (uint8_t)(huart->Instance->RDR & 0x00FFU);

	/* newline -> received command complete */
	if(rxChar == '\n')
	{
		rxBufferReady = 1;

		rxChar = 0;
		inputIndex = 0;
	}
	else
	{
		/* carriage return or 0 */
		if((rxChar == '\r') || (rxChar == 0))
		{
			/* Ignore */
		}
		/* backspace */
		else if(rxChar == '\b')
		{
			if(inputIndex > 0)
			{
				inputIndex--;
				input.buffer[inputIndex] = 0;
			}
		}
		else
		{
			if( inputIndex < MAX_CLI_BUFFER_SIZE )
			{
				input.buffer[inputIndex] = rxChar;
				inputIndex++;
			}
		}
	}
}

//	HAL_UART_Transmit(&huart1, msg, sizeof(msg), 100);
