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

#define MAX_INPUT_LENGTH    50
#define MAX_OUTPUT_LENGTH   100

static const int8_t * const pcWelcomeMessage =
"\
**************************************************************\r\n\
* BS IOT Command Line Interface.                             *\r\n\
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

	int8_t cRxedChar, cInputIndex = 0;
	BaseType_t xMoreDataToFollow;

	const uint32_t UART_TIMEOUT = 500;

	/* The input and output buffers are declared static to keep them off the stack. */
	static int8_t pcOutputString[ MAX_OUTPUT_LENGTH ] = { 0 };
	static int8_t pcInputString[ MAX_INPUT_LENGTH ] = { 0 };

	FreeRTOS_CLIRegisterCommand(&toggleLedCmd);
	FreeRTOS_CLIRegisterCommand(&turnOnLedCmd);
	FreeRTOS_CLIRegisterCommand(&turnOffLedCmd);
	FreeRTOS_CLIRegisterCommand(&clearScreenCmd);


	HAL_UART_Transmit(&huart1, clearScreen, sizeof(clearScreen), UART_TIMEOUT);
	HAL_UART_Transmit(&huart1, resetCursor, sizeof(resetCursor), UART_TIMEOUT);
//	HAL_UART_Receive(&huart1, &cRxedChar, sizeof(cRxedChar), portMAX_DELAY); /* reveid first byte because its an failed byte */

	while (1)
	{

		/* Send a welcome message to the user knows they are connected. */
		HAL_UART_Transmit(&huart1, pcWelcomeMessage, strlen(pcWelcomeMessage), UART_TIMEOUT);

		for( ;; )
		{
			/* This implementation reads a single character at a time.  Wait in the
			Blocked state until a character is received. */
			HAL_UART_Receive(&huart1, &cRxedChar, sizeof(cRxedChar), portMAX_DELAY);

			if( cRxedChar == '\n' )
			{
				/* A newline character was received, so the input command string is
				complete and can be processed.  Transmit a line separator, just to
				make the output easier to read. */
				HAL_UART_Transmit(&huart1, "\r\n", strlen( "\r\n" ), UART_TIMEOUT);

				/* The command interpreter is called repeatedly until it returns
				pdFALSE.  See the "Implementing a command" documentation for an
				exaplanation of why this is. */
				do
				{
					/* Send the command string to the command interpreter.  Any
					output generated by the command interpreter will be placed in the
					pcOutputString buffer. */
					xMoreDataToFollow = FreeRTOS_CLIProcessCommand
								  (
									  pcInputString,   /* The command string.*/
									  pcOutputString,  /* The output buffer. */
									  MAX_OUTPUT_LENGTH/* The size of the output buffer. */
								  );

					/* Write the output generated by the command interpreter to the
					console. */
//					huart1.gState = HAL_UART_STATE_READY;
					HAL_UART_Transmit(&huart1, pcOutputString, strlen( pcOutputString ), 500);

				} while( xMoreDataToFollow != pdFALSE );

				/* All the strings generated by the input command have been sent.
				Processing of the command is complete.  Clear the input string ready
				to receive the next command. */
				cRxedChar = 0;
				cInputIndex = 0;
				memset( pcInputString, 0x00, MAX_INPUT_LENGTH );
			}
			else
			{
				/* The if() clause performs the processing after a newline character
				is received.  This else clause performs the processing if any other
				character is received. */

				if( (cRxedChar == '\r') || (cRxedChar == 0))
				{
					/* Ignore carriage returns. */
				}
				else if( cRxedChar == '\b' )
				{
					/* Backspace was pressed.  Erase the last character in the input
					buffer - if there are any. */
					if( cInputIndex > 0 )
					{
						cInputIndex--;
						pcInputString[ cInputIndex ] = 0;
					}
				}
				else
				{
					/* A character was entered.  It was not a new line, backspace
					or carriage return, so it is accepted as part of the input and
					placed into the input buffer.  When a n is entered the complete
					string will be passed to the command interpreter. */
					if( cInputIndex < MAX_INPUT_LENGTH )
					{
						pcInputString[ cInputIndex ] = cRxedChar;
						cInputIndex++;
					}
				}
			}
		}
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

//	HAL_UART_Transmit(&huart1, msg, sizeof(msg), 100);
