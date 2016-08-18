/*
  Read Temperature for the sensor MPC 9701/A connected ad A0 and Write the value in Serial Port
  Try to implement us to FreeRTOS
 */

#include <Arduino_FreeRTOS.h>
#include <semphr.h>  // ad the FreeRTOS functions for Semaphores (or Flags).


#define VREF 1.1
#define V0 (0.4)
#define DELTAVT (0.0195)
#define STEP (VREF/1024.0)


// These constants won't change.  They're used to give names
// to the pins used:

SemaphoreHandle_t xSerialSemaphore;

const int analogInPinTemperature0 = A0;  // Analog input pin for the Sensor 1 
const int analogInPinTemperature1 = A1;  // Analog input pin for the Sensor 2 now not used

// define two Tasks for DigitalRead & AnalogRead
void TaskTemp1(void *pvParameters);
void TaskTemp2(void *pvParameters);
void readTemperature(int analogPin);



void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(1200);
  analogReference(INTERNAL);
  // Semaphores are useful to stop a Task proceeding, where it should be paused to wait,
  // because it is sharing a resource, such as the Serial port.
  // Semaphores should only be used whilst the scheduler is running, but we can set it up here.
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
	  , 128  // This stack size can be checked & adjusted by reading the Stack Highwater
	  , NULL
	  , 2  // Priority, with 1 being the highest, and 4 being the lowest.
	  , NULL);

  xTaskCreate(
	  TaskTemp2
	  , (const portCHAR *) "T2"
	  , 128  // Stack size
	  , NULL
	  , 1  // Priority
	  , NULL);

  // Now the Task scheduler, which takes over control of scheduling individual Tasks, is automatically started.




}




void TaskTemp1(void *pvParameters __attribute__((unused))) {

	while (true) {
		
		double t;
		int sensorValue;
		long now = millis();
		// read the analog in value:
		sensorValue = analogRead(A0);
		// map it to Celsius Temperature:
		t = ((STEP * sensorValue) - V0) / DELTAVT;

		// change the analog out value:

		// print the results to the serial monitor:
		if (xSemaphoreTake(xSerialSemaphore, (TickType_t)2) == pdTRUE)
		{
			Serial.print("Value for A0 read at: ");
			Serial.print(now);
			Serial.print("\t sensor = ");
			Serial.print(sensorValue);
			Serial.print("\t temp = ");
			Serial.println(t);
			Serial.flush();
		}
		xSemaphoreGive(xSerialSemaphore);
		// wait 2 milliseconds before the next loop
		// for the analog-to-digital converter to settle
		// after the last reading:
		//delay(2);



	vTaskDelay(1);
	}



}

void TaskTemp2(void *pvParameters __attribute__((unused))) {


	while (true) {

		double t;
		long now;
		// read the analog in value:
		int sensorValue = analogRead(A1);
		// map it to Celsius Temperature:
		now = millis();
		t = ((STEP * sensorValue) - V0) / DELTAVT;

		// change the analog out value:
		delay(200);
		// print the results to the serial monitor:
		if (xSemaphoreTake(xSerialSemaphore, (TickType_t)10) == pdTRUE)
		{
			Serial.print("Value for A1 read at: ");
			Serial.print(now);
			Serial.print("\t sensor = ");
			Serial.print(sensorValue);
			Serial.print("\t temp = ");
			Serial.println(t);
			Serial.flush();
		}
		xSemaphoreGive(xSerialSemaphore);
		// wait 2 milliseconds before the next loop
		// for the analog-to-digital converter to settle
		// after the last reading:
		//delay(2);



		vTaskDelay(10);
	}


}





void loop() {
	// not used now

}