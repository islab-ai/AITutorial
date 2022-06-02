/*
  Object classifier by color
  --------------------------

  Uses RGB color sensor input to Neural Network to classify objects
  Outputs object class to serial using unicode emojis

  Note: The direct use of C/C++ pointers, namespaces, and dynamic memory is generally
        discouraged in Arduino examples, and in the future the TensorFlowLite library
        might change to make the sketch simpler.

  Hardware: Arduino Nano 33 BLE Sense board.

  Created by Don Coleman, Sandeep Mistry
  Adapted by Dominic Pajak

  This example code is in the public domain.
*/

// Arduino_TensorFlowLite
#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/version.h>

#include <Arduino_APDS9960.h>
#include "model.h"

// TensorFlow Lite (Micro)에 사용되는 전역변수
tflite::MicroErrorReporter tflErrorReporter;

// 모든 TFLM 작업을 가져옴. 스케치의 컴파일된 사이즈를 줄이려면 이 줄을 제거하고
// 필요한 TFLM 작업만 가져올 수 있음. 
tflite::AllOpsResolver tflOpsResolver;

const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;

// TFLM용 정적 메모리 버퍼를 생성. 사용 중인 모델에 따라 크기를 조정.
constexpr int tensorArenaSize = 8 * 1024;
byte tensorArena[tensorArenaSize];

// gesture index를 name에 매핑하는 array
const char* CLASSES[] = {
  "Banana", // u8"\U0001F34E",
  "Blueberries", // u8"\U0001F34C",
  "Stawberries" // u8"\U0001F34A" 
};

#define NUM_CLASSES (sizeof(CLASSES) / sizeof(CLASSES[0]))

// setup(): Arduino 스케치가 시작될 때 호출. 함수 내에 변수 설정, 핀 모드, 라이브러리 사용 등을 위한 초기화
// Arduino 보드를 켜거나 리셋 후 단 한 번만 실행 
void setup() {

  // Serial.begin(speed): 시리얼 포트 초기화, speed는 시리얼 통신 속도(보-레이트(Baud-Rate) 값 설정) 
  // Serial.begin()함수로 통신속도 설정. while문 내의 Serial은 시리얼 포트가 준비되면 TRUE 반환
  Serial.begin(9600);
  while (!Serial) {};

  Serial.println("Object classification using RGB color sensor");
  Serial.println("--------------------------------------------");
  Serial.println("Arduino Nano 33 BLE Sense running TensorFlow Lite Micro");
  Serial.println("");

  // 센서 초기화 실패하면 오류를 발생
  if (!APDS.begin()) {
    Serial.println("Error initializing APDS9960 sensor.");
  }

  // model byte array의 TFL representation 얻기 
  tflModel = tflite::GetModel(model);
  if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema mismatch!");
    while (1);
  }

  // 모델 실행을 위한 interpreter 생성 
  tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize, &tflErrorReporter);

  // 모델의 입력 및 출력 Tensor에 대한 메모리 할당
  tflInterpreter->AllocateTensors();

  // 모델의 입력 및 출력 Tensor에 대한 포인터 가져옴
  tflInputTensor = tflInterpreter->input(0);
  tflOutputTensor = tflInterpreter->output(0);
}

// loop(): 루프를 반복하여 프로그램이 기능을 수행하고 응답할 수 있도록 함. Arduino 보드를 능동적으로 제어할 수 있음.
void loop() {
  int r, g, b, p, c;
  float sum;

  // 색상 및 근접 데이터 샘플을 모두 사용할 수 있는지 확인
  while (!APDS.colorAvailable() || !APDS.proximityAvailable()) {}

  // 색상 및 근접 센서의 값을 저장
  APDS.readColor(r, g, b, c);
  p = APDS.readProximity();
  sum = r + g + b;

  // object가 가깝고 충분히 밝은지 확인
  if (p == 0 && c > 10 && sum > 0) {

    // normalize
    float redRatio = r / sum;
    float greenRatio = g / sum;
    float blueRatio = b / sum;

    // tensorflow에 센서 데이터 입력
    tflInputTensor->data.f[0] = redRatio;
    tflInputTensor->data.f[1] = greenRatio;
    tflInputTensor->data.f[2] = blueRatio;

    // run inferencing
    TfLiteStatus invokeStatus = tflInterpreter->Invoke();
    if (invokeStatus != kTfLiteOk) {
      Serial.println("Invoke failed!");
      while (1);
      return;
    }

    // output results
    for (int i = 0; i < NUM_CLASSES; i++) {
      Serial.print(CLASSES[i]);
      Serial.print(" ");
      Serial.print(int(tflOutputTensor->data.f[i] * 100));
      Serial.print("%\n");
    }
    Serial.println();

    // wait for the object to be moved away
    while (!APDS.proximityAvailable() || (APDS.readProximity() == 0)) {}
  }

}
