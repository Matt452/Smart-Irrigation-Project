#include "thingProperties.h"
#include "Wire.h"
#include "Adafruit_seesaw.h"

#define PCAADDR 0x70
Adafruit_seesaw ss(&Wire1); // Pass the Wire1 instance to the constructor
int mincap = 400;
int maxcap = 909;
//Relay initialize
const int PumpRelayPin = 0;
const int ValveRelayPin = 1;
bool PumpState = false;
bool sensorsPresent = false;

void pcaselect(uint8_t i) {
  if (i > 7) return;
  Wire1.beginTransmission(PCAADDR);
  Wire1.write(1 << i);
  Wire1.endTransmission();
}

bool SoilSensorCheck(int i, bool initialCheck = false) {
  pcaselect(i);
  if (initialCheck) {
    Serial.print("Searching for Soil Sensor on Port "); Serial.print(i); Serial.println("...");
  }
  if (!ss.begin(0x36)) {
    if (initialCheck) {
      Serial.print("ERROR! seesaw Soil Sensor not found on Port "); Serial.println(i);
    }
    return false; // Return false if sensor not found
  } else {
    if (initialCheck) {
      Serial.print("Soil Sensor started on Port "); Serial.print(i); Serial.println("!");
    }
    return true;  // Return true if sensor found
  }
}

void PumpToggle() {
  // Toggle the state
  digitalWrite(PumpRelayPin, PumpState); // Update relay based on state
  digitalWrite(LED_BUILTIN, !PumpState);
  PumpState = !PumpState;
  if (PumpState == false) {
    Serial.println("Pump off");
  } else {
    Serial.println("Pump on");
  }
}

void ValveChange() {
  digitalWrite(PumpRelayPin, PumpState); // Update relay based on state
  digitalWrite(LED_BUILTIN, !PumpState);
  PumpState = !PumpState;
}

void Water() {
  PumpToggle();
  delay(8000);
  PumpToggle();
}

void SoilTest(int i) {
  pcaselect(i); // Set the current port

  Serial.print("Port: ");
  Serial.println(i);

  float tempC = ss.getTemp();
  uint16_t capread = ss.touchRead(0);
  double percentage = (((double)(capread - mincap) / (double)(maxcap - mincap)) * 100);

  Serial.print(i); // Print the port number
  Serial.print(":Temperature: "); Serial.print(tempC); Serial.println("*C");

  Serial.print(i); // Print the port number
  Serial.print(":Capacitive: "); Serial.println(capread);

  //--------------------Conditional Percent------------------------
  if (percentage > 100) {
    Serial.print(i); // Print the port number
    Serial.println(":Percent: %100");
  } else if (percentage < 0) {
    Serial.print(i); // Print the port number
    Serial.println(":Percent: %0");
  } else {
    Serial.print(i); // Print the port number
    Serial.print(":Percent: %"); Serial.println(percentage);
  }
  if (percentage < 50) {
    Water();
  }
  Serial.println(); // Add an extra newline after the entire SoilTest output for this port
}

void onWaterPumpChange() {
  if (water_Pump) {
    Serial.println("Water pump turned ON from the cloud.");
    digitalWrite(PumpRelayPin, HIGH); // Assuming HIGH turns the pump ON
    PumpState = true;
  } else {
    Serial.println("Water pump turned OFF from the cloud.");
    digitalWrite(PumpRelayPin, LOW);  // Assuming LOW turns the pump OFF
    PumpState = false;
  }
  digitalWrite(LED_BUILTIN, !PumpState); // Update the built-in LED
}

bool CheckSensorsPresent() {
  bool sensor1Present = SoilSensorCheck(0);
  bool sensor2Present = SoilSensorCheck(1);

  if (sensor1Present && sensor2Present) {
    if (!sensorsPresent) {
      Serial.println("Both soil sensors are present.");
      sensorsPresent = true; // Update the flag
    }
    return true;
  } else {
    Serial.println("One or more soil sensors are missing!");
    sensorsPresent = false; // Update the flag
    return false;
  }
}

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(115200); // Use the faster baud rate from the sensor sketch
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500);

  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  Serial.println("Initializing...");
  delay(3000);
  Wire1.begin();

  // Initial sensor check with detailed messages
  bool sensor1Found = SoilSensorCheck(0, true);
  bool sensor2Found = SoilSensorCheck(1, true);

  if (sensor1Found && sensor2Found) {
    sensorsPresent = true;
    Serial.println("Both soil sensors initialized successfully.");
  } else {
    sensorsPresent = false;
    Serial.println("One or more soil sensors not found initially.");
  }

  //-------Relay--------
  pinMode(PumpRelayPin, OUTPUT);
  digitalWrite(PumpRelayPin, HIGH); // Ensure it starts off
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  ArduinoCloud.update();

  if (CheckSensorsPresent()) {
    //Plant 1
    SoilTest(0);
    delay(4000);

    //Plant 2
    SoilTest(1);
    delay(4000);
  } else {
    // Optionally, you can put the board into a low-power state or display a message here
    Serial.println("Sensors not found. Loop is idle.");
    delay(10000); // Wait for a while before checking again (optional)
  }
}