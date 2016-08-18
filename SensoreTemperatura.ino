/*
  Read Temperature for the sensor MPC 9701/A connected ad A0 and Write the value in Serial Port

 */
#define VREF 1.1
#define V0 (0.4)
#define DELTAVT (0.0195)
#define STEP (VREF/1024.0)


// These constants won't change.  They're used to give names
// to the pins used:
const int analogInPinTemperature0 = A0;  // Analog input pin for the Sensor 1 
const int analogInPinTemperature1 = A1;  // Analog input pin for the Sensor 2 now not used

int sensorValue;

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(115200);
  analogReference(INTERNAL);
  
}

void loop() {
  double t;
  // read the analog in value:
  sensorValue = analogRead(analogInPinTemperature0);
  // map it to Celsius Temperature:
  t= ((STEP * sensorValue) - V0)/DELTAVT;
 
  // change the analog out value:
  
  // print the results to the serial monitor:
  Serial.print("sensor = ");
  Serial.print(sensorValue);
  Serial.print("\t output = ");
  Serial.println(t);

  // wait 2 milliseconds before the next loop
  // for the analog-to-digital converter to settle
  // after the last reading:
  delay(2);
}
