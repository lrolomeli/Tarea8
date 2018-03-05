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

/**************************************************
 * GLOBAL VARIABLES
 *************************************************/
SemaphoreHandle_t sem_minutes;
SemaphoreHandle_t sem_hours;

/**************************************************
 * TASKS
 *************************************************/
void countSec_task(void * pvParameters)
{
	TickType_t xLastWakeTime;
	static uint8_t seconds = 0;

	const TickType_t xPeriod = pdMS_TO_TICKS(1000);

	xLastWakeTime = xTaskGetTickCount();

	for(;;)
	{

		if(60 > seconds)
		{
			seconds++;
		}
		else
		{
			seconds = 0;
			xSemaphoreGive(	sem_minutes );
		}

		vTaskDelayUntil(&xLastWakeTime, xPeriod);

	}


}

void countMin_task(void * pvParameters)
{
	static uint8_t minutes = 0;

	sem_minutes = xSemaphoreCreateBinary();

	for(;;)
	{
		xSemaphoreTake( sem_minutes, portMAX_DELAY );

		if(60 > minutes)
		{
			minutes++;
		}
		else
		{
			minutes=0;
			xSemaphoreGive(	sem_hours );
		}

	}


}

void countHour_task(void * pvParameters)
{
	static uint8_t hours = 0;
	sem_hours = xSemaphoreCreateBinary();


	for(;;)
	{
		xSemaphoreTake( sem_hours, portMAX_DELAY );
		hours = (24 > hours) ? (hours + 1) : 0;
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

	xTaskCreate(countSec_task, "sec", 110, (void *) 0, 2, NULL);
	xTaskCreate(countMin_task, "min", 110, (void *) 0, 1, NULL);
	xTaskCreate(countHour_task, "hour", 110, (void *) 0, 0, NULL);

	vTaskStartScheduler();

    for(;;)
    {

    }
    return 0 ;
}
