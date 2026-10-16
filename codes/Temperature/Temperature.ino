#include <Wire.h>

#include "MAX30105.h" 
MAX30105 particleSensor;

void setup()
{
Serial.begin(9600);
Serial.println("Initializing...");

// Initialize sensor
if (particleSensor.begin(Wire, I2C_SPEED_FAST) == false)
{
Serial.println("MAX30105 was not found. Please check wiring/power. ");
while (1);
}
particleSensor.setup(0);
particleSensor.enableDIETEMPRDY(); //Enable the temp ready interrupt. This is required.
}

void loop()
{
float temperature = particleSensor.readTemperature();

Serial.print("temperatureC=");
Serial.print(temperature, 4);

float temperatureF = particleSensor.readTemperatureF();

Serial.print(" temperatureF=");
Serial.print(temperatureF, 4);

Serial.println();
}
