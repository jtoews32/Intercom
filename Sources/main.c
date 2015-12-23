/* ###################################################################
**     Filename    : main.c
**     Project     : Intercom
**     Processor   : MK64FN1M0VLQ12
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2015-12-21, 20:15, # CodeGen: 0
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 01.01
** @brief
**         Main module.
**         This module contains user's application code.
*/         
/*!
**  @addtogroup main_module main module documentation
**  @{
**
**  extern unsigned char __attribute__((section (".cacheBufferSection")))  buffer[100000];
**
**     .cacheBlock 0x20030000 :
  {
    KEEP(*(.cacheBufferSection))
  } > m_data

**
*/         
/* MODULE main */

/* Including needed modules to compile this module/procedure */
#include "Cpu.h"
#include "Events.h"
#include "TI1.h"
#include "TimerIntLdd1.h"
#include "TU1.h"
#include "FRTOS1.h"
#include "UTIL1.h"
#include "WAIT1.h"
#include "FAT1.h"
#include "SD1.h"
#include "SS1.h"
#include "CD1.h"
#include "TmDt1.h"
#include "TMOUT1.h"
#include "CS1.h"
#include "SM2.h"
#include "AD1.h"
#include "AdcLdd1.h"
/* Including shared modules, which are used for whole project */
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
#include "PDD_Includes.h"
#include "Init_Config.h"

static FATFS fileSystemObject;
static FIL fpData;
extern xSemaphoreHandle binaryTimerHandle;
static xSemaphoreHandle startRecordHandle;
static int sampleCounter = 0;
static TaskHandle_t sampleTaskHandle = NULL;
unsigned char cacheBuffer[180000];

void DataLoggerError(void) {
	for (;;) {}
}

static void adcSample(void) {
	if(sampleCounter >= 180000) {
		FRTOS1_xSemaphoreGive( startRecordHandle );
	    if( sampleTaskHandle != NULL ) {
	    	 FRTOS1_vTaskDelete(sampleTaskHandle);
	    }
	  //  sampleCounter = 0;
	} else {
		uint16_t sample= 0;

		AD1_Measure(TRUE);
		AD1_GetValue16(&sample);
		uint8_t msByte = (sample >> 8) & 0x00ff;
		uint8_t lsByte = sample & 0x00ff;

		cacheBuffer[sampleCounter++] = msByte;
		cacheBuffer[sampleCounter++] = lsByte;
	}
}

static void adcRecord(void) {

	int  totalSampleCount = sampleCounter;


	UINT bw = 0;
	int copyCounter = 0;

	if (FAT1_Init() != ERR_OK) {
		DataLoggerError();
	}
	if (FAT1_mount(&fileSystemObject, (const TCHAR*) "0", 1) != FR_OK) {
		DataLoggerError();
	}
	if (FAT1_open(&fpData, "./short1.raw", FA_OPEN_ALWAYS|FA_WRITE) != FR_OK) {
		DataLoggerError();
	}
	if (FAT1_lseek(&fpData, fpData.fsize) != FR_OK || fpData.fptr != fpData.fsize) {
		DataLoggerError();
	}
	while(TRUE) {
		if(copyCounter >= totalSampleCount) {
			break;
		}
		uint8_t audioData[2];
		audioData[0] = cacheBuffer[copyCounter++];
		audioData[1] = cacheBuffer[copyCounter++];
		int length = UTIL1_strlen((uint8_t ) audioData);

		if (FAT1_write(&fpData, audioData, length, &bw) != FR_OK) {
			FAT1_close(&fpData);
			break;
		}
	}
	FAT1_close(&fpData);




	/*
	if (FAT1_Init() != ERR_OK) {
		DataLoggerError();
	}
	if (FAT1_mount(&fileSystemObject, (const TCHAR*) "0", 1) != FR_OK) {
		DataLoggerError();
	}
	if (FAT1_open(&fpData, "./short1.raw", FA_OPEN_ALWAYS|FA_WRITE) != FR_OK) {
		DataLoggerError();
	}
	if (FAT1_lseek(&fpData, fpData.fsize) != FR_OK || fpData.fptr != fpData.fsize) {
		DataLoggerError();
	}
	while(TRUE) {
		if(copyCounter >= totalSampleCount) {
			break;
		}
		uint8_t audioData[2];
		audioData[0] = cacheBuffer[copyCounter++]; // (sample >> 8) & 0x00ff;
		audioData[1] = cacheBuffer[copyCounter++]; // 0; // sample & 0x00ff;
		int length = 2; // UTIL1_strlen((uint8_t *) audioData);

		if (FAT1_write(&fpData, audioData, length, &bw) != FR_OK) {
			FAT1_close(&fpData);
			break;
		}
	}
	FAT1_close(&fpData);

*/
	// copyCounter = 0;

}

void samplerTickTask(void *param) {
	(void)param;
	while(1) {
		if( FRTOS1_xSemaphoreTake(binaryTimerHandle, 1000000) == pdTRUE ) {
			adcSample();
		}
	}
}

void recorderTask(void *param) {
	(void)param;
	while(1) {
		if( FRTOS1_xSemaphoreTake(startRecordHandle, 1000000) == pdTRUE ) {
			adcRecord();
		}
	}
}

/* User includes (#include below this line is not maintained by Processor Expert) */

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /* Write your local variable definition here */
  sampleCounter = 0;
  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/

  binaryTimerHandle = FRTOS1_xSemaphoreCreateBinary();
  startRecordHandle = FRTOS1_xSemaphoreCreateBinary();

  /* Write your code here */
  /* For example: for(;;) { } */
  if (FRTOS1_xTaskCreate(
		 samplerTickTask,
	(signed portCHAR *)"Sampler Tick TiTask",
	configMINIMAL_STACK_SIZE,
	(void*)NULL, tskIDLE_PRIORITY, &sampleTaskHandle ) != pdPASS) {
		/*lint -e527 */
	for(;;){}; /* error! probably out of memory */
		/*lint +e527 */
  }

  if (FRTOS1_xTaskCreate(
		 recorderTask,
	(signed portCHAR *)"Recorder Task",
	configMINIMAL_STACK_SIZE,
	(void*)NULL, tskIDLE_PRIORITY, (xTaskHandle*)NULL ) != pdPASS) {
		/*lint -e527 */
	for(;;){}; /* error! probably out of memory */
		/*lint +e527 */
  }

  FRTOS1_vTaskStartScheduler();
  /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.4 [05.09]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
*/
