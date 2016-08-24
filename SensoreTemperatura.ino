/*
  Read Temperature for the sensor MPC 9701/A connected ad A0 and Write the value in Serial Port
  Try to use FreeRTOS in the version of: https://github.com/greiman/FreeRTOS-Arduino

  Generate a signal to drive a stepper motor

 */


#include <FreeRTOS_AVR.h>
//#include <semphr.h>  // ad the FreeRTOS functions for Semaphores (or Flags).


#define VREF 1.1
#define V0 (0.4)
#define DELTAVT (0.0195)
#define STEP (VREF/1024.0)

#define TDELAYT1  10
#define TDELAYT2  20
#define TDELAYSERIAL configTICK_RATE_HZ
#define TDELAYMOTOR 1
const TickType_t xFrequency = 1;


volatile  uint32_t tmax = 0;
volatile  uint32_t tmin = 0XFFFFFFFF;
volatile  double T1, T2;
volatile  int nReadT1=0, nReadT2=0;
volatile  int np = 0;
volatile  int step = 0;
// These constants won't change.  They're used to give names
// to the pins used:

SemaphoreHandle_t xSerialSemaphore;

const int analogInPinTemperature0 = A0;  // Analog input pin for the Sensor 1 
const int analogInPinTemperature1 = A1;  // Analog input pin for the Sensor 2 now not used
const int ledPin = 13;

// define two Tasks for DigitalRead & AnalogRead
void TaskTemp1(void *pvParameters);
void TaskTemp2(void *pvParameters);
void TaskMotor(void *pvParameters);
void TaskSerial(void *pvParameters);



void setup() {
  // initialize serial communications at 115200 bps:
  Serial.begin(115200);
  analogReference(INTERNAL);
  pinMode(ledPin, OUTPUT);
  /*
   Semaphores are useful to stop a Task proceeding, where it should be paused to wait,
   because it is sharing a resource, such as the Serial port.
   Semaphores should only be used whilst the scheduler is running, but we can set it up here.
   */
  if (xSerialSemaphore == NULL)  // Check to confirm that the Serial Semaphore has not already been created.
  {
	  xSerialSemaphore = xSemaphoreCreateMutex();  // Create a mutex semaphore we will use to manage the Serial Port
	  if ((xSerialSemaphore) != NULL)
		  xSemaphoreGive((xSerialSemaphore));  // Make the Serial Port available for use, by "Giving" the Semaphore.
  }
  
  

  // Now set up two Tasks to run independently.
  xTaskCreate(
	  TaskTemp1
	  , (const portCHAR *)"T1"  // A name just for humans
	  , configMINIMAL_STACK_SIZE + 100 // This stack size can be checked & adjusted by reading the Stack Highwater
	  , NULL
	  , 1  // Priority, with 1 being the highest, and 4 being the lowest.
	  , NULL);

  xTaskCreate(
	  TaskTemp2
	  , (const portCHAR *) "T2"
	  , configMINIMAL_STACK_SIZE + 100   // Stack size
	  , NULL
	  , 1  // Priority
	  , NULL);

  // Now the Task scheduler, which takes over control of scheduling individual Tasks, is automatically started.

  xTaskCreate(
	  TaskMotor
	  , (const portCHAR *) "TMotor"
	  , configMINIMAL_STACK_SIZE + 256  // Stack size
	  , NULL
	  , 2// Priority
	  , NULL);


  xTaskCreate(
	  TaskSerial
	  , (const portCHAR *) "TSerial"
	  , configMINIMAL_STACK_SIZE + 200 // Stack size
	  , NULL
	  , 1 // Priority
	  , NULL);

  // start FreeRTOS
  Serial.println(F("Start"));
  vTaskStartScheduler();

  // should never return
  Serial.println(F("Die"));
  while (1);


}




void TaskTemp1(void *pvParameters __attribute__((unused))) {

	while (true) {

		int sensorValue = analogRead(A0);
		// map it to Celsius Temperature:
		T1 = ((STEP * sensorValue) - V0) / DELTAVT;
		nReadT1++;
		vTaskDelay(TDELAYT1);

	}


}
void TaskTemp2(void *pvParameters __attribute__((unused))) {


	while (true) {

		// read the analog in value:
		int sensorValue = analogRead(A1);
		// map it to Celsius Temperature:
		T2 = ((STEP * sensorValue) - V0) / DELTAVT;
		nReadT2++;
		vTaskDelay(TDELAYT2);
	}


}


void TaskMotor(void *pvParameters __attribute__((unused))) {

		uint32_t tlast = micros();
		bool statoLed = true;
		TickType_t xLastWakeTime;


		// Initialise the xLastWakeTime variable with the current time.
		xLastWakeTime = xTaskGetTickCount();
		for(;;) {
			// Wait for the next cycle.
		    vTaskDelayUntil(&xLastWakeTime, xFrequency);
			//vTaskDelay(TDELAYMOTOR);
			// get wake time
			uint32_t tmp = micros();
			uint32_t diff = tmp - tlast;
			tlast = tmp; 
			if (diff < tmin) tmin = diff;
			if (diff > tmax) tmax = diff;
			
			digitalWrite(ledPin, statoLed);
			statoLed = !statoLed;
			//digitalWrite(ledPin, 0);
			//digitalWrite(ledPin, 1);
			step++;
		}


}
void TaskSerial(void *pvParameters __attribute__((unused))) {
	uint8_t np = 0;
	while (true) {

		if (xSemaphoreTake(xSerialSemaphore, (TickType_t)2) == pdTRUE)

			Serial.print("N Value for A0 read at: ");
			Serial.print(nReadT1);
			nReadT1 = 0;
			Serial.print("\t temp = ");
			Serial.print(T1);
	
			Serial.print("\tN Value for A1 read at: ");
			Serial.print(nReadT2);
			nReadT2 = 0;
			Serial.print("\t temp = ");
			Serial.print(T2);
			Serial.print("\ttmin = ");
			Serial.print(tmin);
			Serial.print("\ttmax = ");
			Serial.print(tmax);
			Serial.print("\tStep = ");
			Serial.println(step);
			if (np++ >= 10) {
				np = 0;
				tmin = 0XFFFFFFFF;
				tmax = 0;
				Serial.println("clear");
				step = 0;
			}
					
			Serial.flush();
			xSemaphoreGive(xSerialSemaphore);
			vTaskDelay(TDELAYSERIAL);
		}

		

	
}


void loop() {
	// not used now

}