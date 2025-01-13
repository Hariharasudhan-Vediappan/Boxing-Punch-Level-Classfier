

#include <Arduino_LSM9DS1.h>

int sample_count = 0;

float max_magnitude = 0;
float energy = 0;

const float accelerationThreshold = 2.0; 
const int numSamples = 100;

int samplesRead = numSamples;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  // print the header
  Serial.println("aX,aY,aZ");
}

void loop() {
  float aX, aY, aZ;

  while (samplesRead == numSamples) {
    if (IMU.accelerationAvailable()) {

      IMU.readAcceleration(aX, aY, aZ);


      float aSum = fabs(aX) + fabs(aY) + fabs(aZ);

      if (aSum >= accelerationThreshold) {
        samplesRead = 0;
        break;
      }
    }
  }

  while (samplesRead < numSamples) {
    if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
      IMU.readAcceleration(aX, aY, aZ);

      samplesRead++;

      Serial.print(aX, 3);
      Serial.print(',');
      Serial.print(aY, 3);
      Serial.print(',');
      Serial.print(aZ, 3);
      Serial.println();



      if (samplesRead == numSamples) {
        Serial.println();
      }
    }
  }
}