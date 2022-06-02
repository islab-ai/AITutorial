// build-in 근접 센서 및 광 센서 사용을 위한 라이브러리
#include <Arduino_APDS9960.h>

// setup(): Arduino 스케치가 시작될 때 호출. 함수 내에 변수 설정, 핀 모드, 라이브러리 사용 등을 위한 초기화
// Arduino 보드를 켜거나 리셋 후 단 한 번만 실행 
void setup() {

  // Serial.begin(speed): 시리얼 포트 초기화, speed는 시리얼 통신 속도(보-레이트(Baud-Rate) 값 설정) 
  // Serial.begin()함수로 통신속도 설정. while문 내의 Serial은 시리얼 포트가 준비되면 TRUE 반환
  Serial.begin(9600);
  while (!Serial) {};

  // 센서 초기화 실패하면 오류를 발생시킴
  if (!APDS.begin()) {
    Serial.println("Error initializing APDS9960 sensor.");
  }

  // print the header
  Serial.println("Red,Green,Blue");
}

// loop(): 루프를 반복하여 프로그램이 기능을 수행하고 응답할 수 있도록 함. Arduino 보드를 능동적으로 제어할 수 있음.
void loop() {
  int r, g, b, c, p;
  float sum;

  // wait for proximity and color sensor data
  while (!APDS.colorAvailable() || !APDS.proximityAvailable()) {}

  // read the color and proximity data
  APDS.readColor(r, g, b, c); // 색상 데이터 저장
  sum = r + g + b;
  p = APDS.readProximity();  // 근접 센서의 값을 변수에 저장

  // object가 가깝고 색상이 충분한지 확인
  if (p == 0 && c > 10 && sum > 0) {

    float redRatio = r / sum;
    float greenRatio = g / sum;
    float blueRatio = b / sum;

    // print the data in CSV format
    Serial.print(redRatio, 3);
    Serial.print(',');
    Serial.print(greenRatio, 3);
    Serial.print(',');
    Serial.print(blueRatio, 3);
    Serial.println();
  }
}
