/*
  IMU Capture
  This example uses the on-board IMU to start reading acceleration and gyroscope
  data from on-board IMU and prints it to the Serial Monitor for one second
  when the significant motion is detected.
  You can also use the Serial Plotter to graph the data.
  The circuit:
  - Arduino Nano 33 BLE or Arduino Nano 33 BLE Sense board.
  Created by Don Coleman, Sandeep Mistry
  Modified by Dominic Pajak, Sandeep Mistry
  This example code is in the public domain.
*/
// Build-in IMU 센서 사용을 위한 라이브러리
#include <Arduino_LSM9DS1.h>

const float accelerationThreshold = 2.5; // threshold of significant in G's
const int numSamples = 119;

int samplesRead = numSamples;

// setup(): Arduino 스케치가 시작될 때 호출. 함수 내에 변수 설정, 핀 모드, 라이브러리 사용 등을 위한 초기화
// Arduino 보드를 켜거나 리셋 후 단 한 번만 실행 
void setup() {
  // Serial.begin(speed): 시리얼 포트 초기화, speed는 시리얼 통신 속도(보-레이트(Baud-Rate) 값 설정) 
  // Serial.begin()함수로 통신속도 설정. while문 내의 Serial은 시리얼 포트가 준비되면 TRUE 반환
  Serial.begin(9600);
  while (!Serial);

  // IMU 센서 초기화. 초기화 실패하면 오류 발생
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  // print the header
  Serial.println("aX,aY,aZ,gX,gY,gZ");
}

// loop(): 루프를 반복하여 프로그램이 기능을 수행하고 응답할 수 있도록 함. Arduino 보드를 능동적으로 제어할 수 있음.
void loop() {
  float aX, aY, aZ, gX, gY, gZ;

  // wait for significant motion
  while (samplesRead == numSamples) {
    if (IMU.accelerationAvailable()) { // 가속도 센서의 값을 사용할 수 있는 경우 1, 아니면 0
      // read the acceleration data
      IMU.readAcceleration(aX, aY, aZ); // aX, aY, aZ에 각 축별 데이터 넣기

      // sum up the absolutes
      // fabs() 함수는 절대값을 리턴
      float aSum = fabs(aX) + fabs(aY) + fabs(aZ);

      // check if it's above the threshold
      if (aSum >= accelerationThreshold) {
        // reset the sample read count
        samplesRead = 0;
        break;
      }
    }
  }

  // check if the all the required samples have been read since
  // the last time the significant motion was detected
  while (samplesRead < numSamples) {
    // 새로운 가속도 및 자이로스코프 데이터를 사용할 수 있는지 확인 
    // 가속도 센서의 값과 자이로 센서의 값 저장
    if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
      // read the acceleration and gyroscope data
      IMU.readAcceleration(aX, aY, aZ);
      IMU.readGyroscope(gX, gY, gZ);

      samplesRead++;

      // print the data in CSV format
      Serial.print(aX, 3);
      Serial.print(',');
      Serial.print(aY, 3);
      Serial.print(',');
      Serial.print(aZ, 3);
      Serial.print(',');
      Serial.print(gX, 3);
      Serial.print(',');
      Serial.print(gY, 3);
      Serial.print(',');
      Serial.print(gZ, 3);
      Serial.println();

      if (samplesRead == numSamples) {
        // add an empty line if it's the last sample
        Serial.println();
      }
    }
  }
}
