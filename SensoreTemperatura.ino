/*
  Read Temperature for the sensor MPC 9701/A connected ad A0 and Write the value in Serial Port
  Try to use FreeRTOS in the version of: https://github.com/greiman/FreeRTOS-Arduino
    
  Generate a signal to drive a stepper motor

 */


#include <FreeRTOS_AVR.h>
#include <Esplora.h>
#include <TFT.h>            // Arduino LCD library
#include <SPI.h>
//#include <FreeRTOS_ARM.h>
//#include <semphr.h>  // ad the FreeRTOS functions for Semaphores (or Flags).


#define VREF 5.0
#define V0 (0.4)
#define DELTAVT (0.0195)
#define STEP (VREF/1024.0)

#define TDELAYT1  10
#define TDELAYT2  20
#define TDELAYSERIAL configTICK_RATE_HZ
#define TDELAYMOTOR 5
#define TDELAYSERIALR configTICK_RATE_HZ
#define TDELAYDISPLAY configTICK_RATE_HZ /4
#define ENABLE_STEPPER 8
#define Z_DIR 3		// out 3	vecchio valore 7
#define Z_STEP 6	// out 2	vecchio valore 11
const TickType_t xFrequency = 4;

//TFT pippo;
volatile  uint32_t tmax = 0;
volatile  uint32_t tmin = 0XFFFFFFFF;
volatile  int T1;
volatile  double T2;
volatile  int nReadT1=0, nReadT2=0;
volatile  int np = 0;
volatile  int barra=0;
volatile  long int step;
volatile  long int stepToDo;
volatile  long int acc[10];
volatile  bool start = false;
volatile  long int tick;
String dato = "        ";
String temperature = "T ?";

// These constants won't change.  They're used to give names
// to the pins used:

SemaphoreHandle_t xSerialSemaphore;
char tempPrintout[3] = {'a','b','\0'};

//const int analogInPinTemperature0 = A0;  // Analog input pin for the Sensor 1 
//const int analogInPinTemperature1 = A1;  // Analog input pin for the Sensor 2 now not used
const int ledPin = 13;





// define two Tasks for DigitalRead & AnalogRead
void TaskTemp1(void *pvParameters);
void TaskTemp2(void *pvParameters);
void TaskMotor(void *pvParameters);
void TaskSerialTrace(void *pvParameters);
void TaskSerialRead(void *pvParameters);
void TaskDisplay(void *pvParameters);

void setup() {
  // initialize serial communications at 115200 bps:
  Serial.begin(115200);
    // while (!Serial);
  //analogReference(INTERNAL);
  pinMode(ledPin, OUTPUT);
  //analogReadResolution(10);
  //analogReference(INTERNAL);
  pinMode(ENABLE_STEPPER, OUTPUT);
  digitalWrite(ENABLE_STEPPER, HIGH);
  pinMode(Z_DIR, OUTPUT);
  pinMode(Z_STEP, OUTPUT);
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
	  , configMINIMAL_STACK_SIZE  // This stack size can be checked & adjusted by reading the Stack Highwater
	  , NULL
	  , 2  // Priority, with 1 being the highest, and 4 being the lowest.
	  , NULL);

  xTaskCreate(
	  TaskTemp2
	  , (const portCHAR *) "T2"
	  , configMINIMAL_STACK_SIZE    // Stack size
	  , NULL
	  , 2  // Priority
	  , NULL);

  // Now the Task scheduler, which takes over control of scheduling individual Tasks, is automatically started.

  xTaskCreate(
	  TaskMotor
	  , (const portCHAR *) "TMotor"
	  , configMINIMAL_STACK_SIZE  + 100// Stack size
	  , NULL
	  , 3// Priority
	  , NULL);


  xTaskCreate(
	  TaskSerialTrace
	  , (const portCHAR *) "TSerT"
	  , configMINIMAL_STACK_SIZE + 200 // Stack size
	  , NULL
	  , 1 // Priority
	  , NULL);

  xTaskCreate(
	  TaskDisplay
	  , (const portCHAR *) "TDisp"
	  , configMINIMAL_STACK_SIZE + 200 // Stack size
	  , NULL
	  , 1 // Priority
	  , NULL);

  xTaskCreate(
	  TaskSerialRead
	  , (const portCHAR *) "TSerR"
	  , configMINIMAL_STACK_SIZE + 100 // Stack size
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
		T1  = Esplora.readTemperature(DEGREES_C);
		nReadT1++;
		barra = 1023 - Esplora.readSlider();
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
		int direction;
	//	long int tick;
		int passo;

		// Initialise the xLastWakeTime variable with the current time.
		xLastWakeTime = xTaskGetTickCount();
		for(;;) {
			// Wait for the next cycle.
		    vTaskDelayUntil(&xLastWakeTime, xFrequency);
			//vTaskDelay(TDELAYMOTOR);
			uint32_t tmp = micros();
			uint32_t diff = tmp - tlast;
			tlast = tmp;
			if (diff < tmin) tmin = diff;
			if (diff > tmax) tmax = diff;


			if (start) {
				start = false;
				step = abs(stepToDo);
				digitalWrite(ENABLE_STEPPER, LOW);
				if (stepToDo < 0)   digitalWrite(Z_DIR, LOW); else digitalWrite(Z_DIR, HIGH);
				tick = 0;
			}
			if (step > 0) {
				if (step < acc[0]) passo = 6;
				if (step >= acc[0] && step < acc[1]) passo = 5;
				if (step >= acc[1] && step < acc[2]) passo = 4;
				if (step >= acc[2] && step < acc[3]) passo = 3;
				if (step >= acc[3] && step < acc[4]) passo = 2;
				if (step >= acc[4] && step < acc[5]) passo = 1;
				if (step >= acc[5] && step < acc[6]) passo = 2;
				if (step >= acc[6] && step < acc[7]) passo = 3;
				if (step >= acc[7] && step < acc[8]) passo = 4;
				if (step >= acc[8] && step < acc[9]) passo = 5;
				if (step >= acc[9]) passo = 6;
				
				if (((tick % passo) == 0)) {
					digitalWrite(Z_STEP, 1);
					digitalWrite(Z_STEP, 1);
					digitalWrite(Z_STEP, 0);
					digitalWrite(ledPin, statoLed);
					statoLed = !statoLed;
					

					step--;
				}
				tick++;
			}
			if (step == 0) {
				digitalWrite(ENABLE_STEPPER, HIGH);
				
			}
			// get wake time
			

			
		}


}

void TaskSerialTrace(void *pvParameters __attribute__((unused))) {
	uint8_t np = 0;
	vTaskDelay(TDELAYSERIAL / 2);
	while (true) {

		if (xSemaphoreTake(xSerialSemaphore, (TickType_t)25) == pdTRUE)

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
			Serial.print(step);

			Serial.print("\ttick = ");
			Serial.println(tick);
			if (np++ >= 10) {
				np = 0;
				tmin = 0XFFFFFFFF;
				tmax = 0;
				Serial.println("clear");
				
			}
					
			//Serial.flush();
			xSemaphoreGive(xSerialSemaphore);
			vTaskDelay(TDELAYSERIAL);
		}

		

	
}


void TaskSerialRead(void *pvParameters __attribute__((unused))) {
	char c;
	long int instep=32000;
	

	while (true) {
		vTaskDelay(TDELAYSERIALR); 
		//dato = "";
		if (xSemaphoreTake(xSerialSemaphore, (TickType_t)20) == pdTRUE) {
		    dato = "";
			while (Serial.available() > 0) {
				c = Serial.read();
				Serial.print(c);
				dato += c;
			}
			//instep = dato.toInt();
			if (dato !="") {
				Serial.print("dato letto = ");
				Serial.print(dato);
			}
			instep = dato.toInt();
			//Serial.flush();
		}
		xSemaphoreGive(xSerialSemaphore);
		if (instep) {
			stepToDo = instep;
			instep = abs(instep);
			int rampa = instep /100;
			if (rampa > 10) rampa = 10;
			acc[0] = 5;
			acc[1] = acc[0] + rampa;
			acc[2] = acc[1] + rampa;
			acc[3] = acc[2] + rampa;
			acc[4] = acc[3] + rampa*4;
			acc[5] = instep - acc[4];
			acc[6] = instep - acc[3];
			acc[7] = instep - acc[2];
			acc[8] = instep - acc[1];
			acc[9] = instep - acc[0];
			noInterrupts();
				start = true;
			interrupts();
			instep = 0;
		}
	}





}

void TaskDisplay(void *pvParameters __attribute__((unused))) {

	// Put this line at the beginning of every sketch that uses the GLCD
	EsploraTFT.begin();
	//clear the screen with a black background
	EsploraTFT.background(0, 0, 0);
	// set the text color to magenta
	EsploraTFT.stroke(200, 20, 180);
	// set the text to size 
	EsploraTFT.setTextSize(1);
	// start the text at the top left of the screen
	// this text is going to remain static
	EsploraTFT.text("Temp. in gradi C :\n ", 0, 0);

	// set the text in the loop to size 5
    EsploraTFT.setTextSize(2);
	//EsploraTFT.setTextWrap(false);
	// 


	int T1Old = T1;
	int barraOld = barra;
	int stepOld = step;
	EsploraTFT.stroke(255, 255, 255);
	EsploraTFT.setCursor(0, 30);
	EsploraTFT.print(T1Old);
	for (;;) 	
	{
		 //TFT pippo;
		 //pippo.w
		    
			vTaskDelay(TDELAYDISPLAY);
			//EsploraTFT.stroke(0, 0, 0);
			//EsploraTFT.text(tempPrintout, 0, 30);
			// set the text color to white
			if (T1Old != T1) {
				EsploraTFT.stroke(0, 0, 0);
				//EsploraTFT.print("lillo");
				EsploraTFT.setCursor(0, 30);
				EsploraTFT.print(T1Old);
				EsploraTFT.stroke(255, 255, 255);
				EsploraTFT.setCursor(0, 30);
				
				T1Old = T1;
				EsploraTFT.print(T1Old);
			};
			if (barraOld != barra) {
				EsploraTFT.stroke(0, 0, 0);
				//EsploraTFT.print("lillo");
				
				EsploraTFT.setCursor(0, 70);
				EsploraTFT.print(barraOld);
				EsploraTFT.line(0, 60, EsploraTFT.width(), 60);
				
				barraOld = barra;
				int graphwidth = map(barraOld, 0, 1023, 0, EsploraTFT.width());
				EsploraTFT.stroke(255, 255, 255);
				EsploraTFT.line(0, 60, graphwidth,60);
				EsploraTFT.setCursor(0, 70);
				EsploraTFT.print(barraOld);
			}
			if (stepOld != step) {
				EsploraTFT.stroke(0, 0, 0);
				//EsploraTFT.print("lillo");

				EsploraTFT.setCursor(0, 95);
				EsploraTFT.print(stepOld);
				EsploraTFT.line(0, 90, EsploraTFT.width(), 90);
				stepOld = step;
				int stepwidth = map(stepOld, 0, stepToDo  , 0, EsploraTFT.width());
				EsploraTFT.stroke(255, 255, 255);
				EsploraTFT.line(0, 90, stepwidth, 90);
				EsploraTFT.setCursor(0, 95);
				EsploraTFT.print(stepOld);
			}
			// print the temperature one line below the static text
			//temperature.toCharArray(tempPrintout, 3);
			//EsploraTFT.text(tempPrintout, 0, 30);
		    // erase the text for the next loop
			//EsploraTFT.stroke(0, 0, 0);
			
		}
}


void loop() {
	// not used now

}