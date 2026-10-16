#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

MAX30105 particleSensor;

#define MAX_BRIGHTNESS 255

uint16_t irBuffer[100]; // Infrared LED sensor data
uint16_t redBuffer[100]; // Red LED sensor data

int32_t bufferLength;    // Data length
int32_t spo2;            // SPO2 value
int8_t validSPO2;        // Indicator to show if the SPO2 calculation is valid
int32_t heartRate;       // Heart rate value
int8_t validHeartRate;   // Indicator to show if the heart rate calculation is valid

byte pulseLED = 11; // Must be on PWM pin
byte readLED = 13;  // Blinks with each data read

// Function to smooth the heart rate using a moving average filter
int32_t smoothHeartRate(int32_t hr) {
    static int32_t hrBuffer[10] = {0};
    static int index = 0;
    static int sum = 0;

    sum -= hrBuffer[index];
    hrBuffer[index] = hr;
    sum += hrBuffer[index];
    index = (index + 1) % 10;

    return sum / 10; // Return the moving average
}

void setup() {
    Serial.begin(115200); // Initialize serial communication at 115200 bits per second:

    pinMode(pulseLED, OUTPUT);
    pinMode(readLED, OUTPUT);

    // Initialize sensor
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) { // Use default I2C port, 400kHz speed
        Serial.println(F("MAX30105 was not found. Please check wiring/power."));
        while (1);
    }

    Serial.println(F("Attach sensor to finger with rubber band. Press any key to start conversion"));
    while (Serial.available() == 0); // Wait until user presses a key
    Serial.read();

    byte ledBrightness = 60; // Options: 0=Off to 255=50mA
    byte sampleAverage = 4;  // Options: 1, 2, 4, 8, 16, 32
    byte ledMode = 2;        // Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
    byte sampleRate = 100;   // Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
    int pulseWidth = 411;    // Options: 69, 118, 215, 411
    int adcRange = 4096;     // Options: 2048, 4096, 8192, 16384

    particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); // Configure sensor with these settings
}

void loop() {
    bufferLength = 100; // Buffer length of 100 stores 4 seconds of samples running at 25sps

    // Read the first 100 samples, and determine the signal range
    for (byte i = 0; i < bufferLength; i++) {
        while (particleSensor.available() == false) // Do we have new data?
            particleSensor.check(); // Check the sensor for new data

        redBuffer[i] = particleSensor.getRed();
        irBuffer[i] = particleSensor.getIR();
        particleSensor.nextSample(); // We're finished with this sample so move to next sample

        Serial.print(F("red="));
        Serial.print(redBuffer[i], DEC);
        Serial.print(F(", ir="));
        Serial.println(irBuffer[i], DEC);
    }

    // Calculate heart rate and SpO2 after first 100 samples
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

    // Smooth the heart rate using a moving average filter
    heartRate = smoothHeartRate(heartRate);

    // Apply constraints to ensure valid heart rate
    if (heartRate < 60 || heartRate > 100) {
        validHeartRate = 0; // Mark heart rate as invalid
    }

    // Continuously take samples from MAX30102
    while (1) {
        // Dump the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
        for (byte i = 25; i < 100; i++) {
            redBuffer[i - 25] = redBuffer[i];
            irBuffer[i - 25] = irBuffer[i];
        }

        // Take 25 sets of samples before calculating the heart rate
        for (byte i = 75; i < 100; i++) {
            while (particleSensor.available() == false) // Do we have new data?
                particleSensor.check(); // Check the sensor for new data

            digitalWrite(readLED, !digitalRead(readLED)); // Blink onboard LED with every data read

            redBuffer[i] = particleSensor.getRed();
            irBuffer[i] = particleSensor.getIR();
            particleSensor.nextSample(); // We're finished with this sample so move to next sample
        }

        // Recalculate HR and SPO2 after gathering 25 new samples
        maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

        // Smooth the heart rate using a moving average filter
        heartRate = smoothHeartRate(heartRate);

        // Check and display results
        Serial.print(F("HR="));
        Serial.print(heartRate, DEC);

        if (validHeartRate && heartRate >= 60 && heartRate <= 100) {
            Serial.print(F(" (Normal)"));
        } else if (heartRate >= 101 && heartRate <= 200) {
            Serial.print(F(" (High) - Medical Attention Needed!"));
        } else if (heartRate < 60) {
            Serial.print(F(" (Low) - Medical Attention Needed!"));
        } else {
            Serial.print(F(" (Unstable) - Sensor Readings Issue!"));
        }

        Serial.print(F(", SPO2="));
        Serial.print(spo2, DEC);

        if (validSPO2 && spo2 >= 90 && spo2 <= 100) {
            Serial.print(F(" (Normal)"));
        } else if (spo2 >= 101 && spo2 <= 120) {
            Serial.print(F(" (High) - Medical Attention Needed!"));
        } else if (spo2 > 120) {
            Serial.print(F(" (Unstable) - Sensor Readings Issue!"));
        } else if (spo2 >= 40 && spo2 <= 50) {
            Serial.print(F(" (Low) - Medical Attention Needed!"));
        } else if (spo2 < 40) {
            Serial.print(F(" (Unstable) - Sensor Readings Issue!"));
        } else {
            Serial.print(F(" (Unstable) - Sensor Readings Issue!"));
        }

        Serial.println();
    }
}
