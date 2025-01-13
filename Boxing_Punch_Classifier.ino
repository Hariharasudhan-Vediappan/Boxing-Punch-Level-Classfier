#include <TensorFlowLite.h>           
#include "model.h"                   
#include <Arduino_LSM9DS1.h>          
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/version.h>


#define BUFFER_SIZE 100

bool loopEnded = false; 

float prediction = 0;

int predicted_class = 0;


float aX_buffer[BUFFER_SIZE];
float aY_buffer[BUFFER_SIZE];
float aZ_buffer[BUFFER_SIZE];

const float mean_max_magnitude = 3.44815727;
const float std_max_magnitude = 1.96905085;

const float mean_energy = 334.4712977;
const float std_energy = 174.93997144;

int sample_count = 0;

float max_mag_tensor = 0;
float energy_tensor = 0;

float max_magnitude = 0;
float energy = 0;

const float accelerationThreshold = 2.0; 
const int numSamples = 100;

int samplesRead = numSamples;

constexpr int tensorArenaSize = 8 * 1024;
byte tensorArena[tensorArenaSize] __attribute__((aligned(16)));


tflite::MicroErrorReporter tflErrorReporter;


tflite::AllOpsResolver tflOpsResolver;

const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  tflModel = tflite::GetModel(g_model);
  if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema mismatch!");
    while (1);
  }

  tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize, &tflErrorReporter);

  tflInterpreter->AllocateTensors();

  tflInputTensor = tflInterpreter->input(0);
  tflOutputTensor = tflInterpreter->output(0);
  Serial.println("Setup complete. Ready for real-time inference.");


}

void loop() {
  float aX, aY, aZ, gX, gY, gZ;


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


      aX_buffer[sample_count] = aX;
      aY_buffer[sample_count] = aY;
      aZ_buffer[sample_count] = aZ;

      sample_count++;

      if (sample_count >= BUFFER_SIZE) {

          sample_count = 0;
          max_magnitude = 0;
          energy = 0;

          for (int i = 0; i < BUFFER_SIZE; i++) {
              float magnitude = sqrt(
                  aX_buffer[i] * aX_buffer[i] +
                  aY_buffer[i] * aY_buffer[i] +
                  aZ_buffer[i] * aZ_buffer[i]
              );

              if (magnitude > max_magnitude) {
                  max_magnitude = magnitude;
              }

              energy += magnitude * magnitude;
          }
      }


      if (samplesRead == numSamples) {


        Serial.println();
        Serial.print("Max_magnitude: ");
        Serial.println(max_magnitude);
        Serial.print("Energy: ");
        Serial.println(energy);



        tflInputTensor->data.f[0] = standardizeMaxMagnitude(max_magnitude);
        tflInputTensor->data.f[1] = standardizeEnergy(energy);

        if (tflInterpreter->Invoke() != kTfLiteOk) {
          Serial.println("Inference failed!");
          return;
        }

        prediction = tflOutputTensor->data.f[0];


        if (prediction > 0.8) {
          Serial.println("Classified as: Light Punch");
        } else if (prediction > 0.4 && prediction < 0.8){
          Serial.println("Classified as: Medium Punch");
        }else {
          Serial.println("Classified as: Heavy Punch");
        }

      }

    }
  }
}

float standardizeMaxMagnitude(float &max_mag) {
    max_mag = (max_mag - mean_max_magnitude) / std_max_magnitude;
    //Serial.println(max_mag);
    return max_mag;
}
float standardizeEnergy(float &eng) {
    eng = (eng - mean_energy) / std_energy;
    //Serial.println(eng);
    return eng;
}