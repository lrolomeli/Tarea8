/*
 * Copyright (c) 2017, NXP Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file    ClaseRelojAlarma.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"
#include "freeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"

#define MAILBOX_LENGTH 1
#define MESSAGE_SIZE sizeof(time_msg_t)
#define PRESET_SEC 59
#define PRESET_MIN 59
#define PRESET_HOUR 23

#define ALARM_SECONDS 6
#define ALARM_MINUTES 0
#define ALARM_HOURS 0

#define EVENT_ALARM_SECONDS (1 << 0)
#define EVENT_ALARM_MINUTES (1 << 1)
#define EVENT_ALARM_HOURS 	(1 << 2)


typedef enum
{
	seconds_type, minutes_type, hours_type

} time_types_t;

typedef struct
{
	time_types_t time_type;
	uint8_t value;

} time_msg_t;

/**************************************************
 * GLOBAL VARIABLES
 *************************************************/
SemaphoreHandle_t sem_minutes;
SemaphoreHandle_t sem_hours;
QueueHandle_t mailbox;
EventGroupHandle_t time_events_g;
/**************************************************
 * TASKS
 *************************************************/
void countSec_task(void * pvParameters)
{
	TickType_t xLastWakeTime;
	static time_msg_t seconds = {seconds_type, PRESET_SEC};

	const TickType_t xPeriod = pdMS_TO_TICKS(1000);

	xLastWakeTime = xTaskGetTickCount();

	for(;;)
	{
		if(ALARM_SECONDS == seconds.value)
		{
			//xEventGroupSetBits(xEventGroup, uxBitsToSet);
			xEventGroupSetBits(time_events_g, EVENT_ALARM_SECONDS);

		}

		if(60 == seconds.value)
		{
			seconds.value = 0;
			xSemaphoreGive(	sem_minutes );
		}
		xQueueSendToBack(mailbox, &seconds, portMAX_DELAY);
		seconds.value++;
		vTaskDelayUntil(&xLastWakeTime, xPeriod);

	}


}

void countMin_task(void * pvParameters)
{
	static time_msg_t minutes = {minutes_type, PRESET_MIN};

	sem_minutes = xSemaphoreCreateBinary();

	for(;;)
	{

		if(ALARM_MINUTES == minutes.value)
		{
			//xEventGroupSetBits(xEventGroup, uxBitsToSet);
			xEventGroupSetBits(time_events_g, EVENT_ALARM_MINUTES);
		}
		xSemaphoreTake( sem_minutes, portMAX_DELAY );
		minutes.value++;


		if(60 == minutes.value)
		{
			minutes.value = 0;
			xQueueSendToBack(mailbox, &minutes, portMAX_DELAY);
			xSemaphoreGive(	sem_hours );
		}
		else
		{
			xQueueSendToBack(mailbox, &minutes, portMAX_DELAY);
		}





	}


}

void countHour_task(void * pvParameters)
{
	static time_msg_t hours = {hours_type, PRESET_HOUR};
	sem_hours = xSemaphoreCreateBinary();

	for(;;)
	{
		if(ALARM_HOURS == hours.value)
		{
			//xEventGroupSetBits(xEventGroup, uxBitsToSet);
			xEventGroupSetBits(time_events_g, EVENT_ALARM_HOURS);
		}
		xSemaphoreTake( sem_hours, portMAX_DELAY );
		hours.value++;

		hours.value = (24 == hours.value) ? 0 : hours.value;
		xQueueSendToBack(mailbox, &hours, portMAX_DELAY);

	}


}
void alarm_task(void * pvParameters)
{
	for(;;)
	{
		//xEventGroupWaitBits(xEventGroup, uxBitsToWaitFor, xClearOnExit, xWaitForAllBits, xTicksToWait);
		xEventGroupWaitBits(time_events_g, EVENT_ALARM_SECONDS|EVENT_ALARM_MINUTES|EVENT_ALARM_HOURS, pdTRUE, pdTRUE, portMAX_DELAY);

		PRINTF("alarm");
	}
}

void print_task(void * pvParameters)
{

	static uint8_t segundo = PRESET_SEC;
	static uint8_t minuto = PRESET_MIN;
	static uint8_t hora = PRESET_HOUR;
	time_msg_t xmessage;

	for(;;)
	{

		xQueueReceive(mailbox, &xmessage, portMAX_DELAY);

		if(seconds_type == xmessage.time_type)
		{
			segundo = xmessage.value;
		}

		else if(minutes_type == xmessage.time_type)
		{
			minuto = xmessage.value;
		}

		else if(hours_type == xmessage.time_type)
		{
			hora = xmessage.value;
		}


		if(10 <= hora && 10 <= minuto && 10 <= segundo)
		{
			PRINTF("%d : %d : %d\r\n", hora, minuto, segundo);
		}
		else if(10 <= hora && 10 <= minuto && 10 > segundo)
		{
			PRINTF("%d : %d : 0%d\r\n", hora, minuto, segundo);
		}
		else if(10 <= hora && 10 > minuto && 10 <= segundo)
		{
			PRINTF("%d : 0%d : %d\r\n", hora, minuto, segundo);
		}
		else if(10 <= hora && 10 > minuto && 10 > segundo)
		{
			PRINTF("%d : 0%d : 0%d\r\n", hora, minuto, segundo);
		}
		else if(10 > hora && 10 > minuto && 10 > segundo)
		{
			PRINTF("0%d : 0%d : 0%d\r\n", hora, minuto, segundo);
		}
		else if(10 > hora && 10 > minuto && 10 <= segundo)
		{
			PRINTF("0%d : 0%d : %d\r\n", hora, minuto, segundo);
		}
		else if(10 > hora && 10 <= minuto && 10 > segundo)
		{
			PRINTF("0%d : %d : 0%d\r\n", hora, minuto, segundo);
		}
		else if(10 > hora && 10 <= minuto && 10 <= segundo)
		{
			PRINTF("0%d : %d : %d\r\n", hora, minuto, segundo);
		}

	}


}

/**************************************************
 * MAIN CODE
 *************************************************/
int main(void) {

  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
  	/* Init FSL debug console. */
    BOARD_InitDebugConsole();

	xTaskCreate(countSec_task, "sec", 110, (void *) 0, 4, NULL);
	xTaskCreate(countMin_task, "min", 110, (void *) 0, 3, NULL);
	xTaskCreate(countHour_task, "hour", 110, (void *) 0, 2, NULL);
	xTaskCreate(alarm_task, "alarm", 110, (void *) 0, 1, NULL);
	xTaskCreate(print_task, "print", 110, (void *) 0, 0, NULL);

	sem_minutes = xSemaphoreCreateBinary();
	sem_hours = xSemaphoreCreateBinary();

	mailbox = xQueueCreate(MAILBOX_LENGTH, MESSAGE_SIZE);

	time_events_g = xEventGroupCreate();

	vTaskStartScheduler();

    for(;;)
    {

    }
    return 0 ;
}
