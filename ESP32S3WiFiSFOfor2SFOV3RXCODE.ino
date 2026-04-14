// 250413 Version3 SFO for 2 servo CODE by K.Kakuta
// ESP-NOW RXで受け取ったch1〜ch5を使う版 RX

#include <WiFi.h>
#include <esp_now.h>
#include "esp_wifi.h"
#include <ESP32Servo.h>

// ====== ESP-NOWで受け取る構造体 ======
typedef struct {
  uint16_t ch1;
  uint16_t ch2;
  uint16_t ch3;
  uint16_t ch4;
  uint16_t ch5;
  uint16_t ch6;
  uint16_t ch7;
  uint16_t ch8;
} PpmPacket;

// 受信値（μsec）を保持する変数（ch1〜ch5を使用）
volatile uint16_t ch1Value = 1500; // Ch1
volatile uint16_t ch2Value = 1500; // Ch2
volatile uint16_t ch3Value = 1000; // Ch3（スロットル/フラップパワー）
volatile uint16_t ch4Value = 1500; // Ch4
volatile uint16_t ch5Value = 1500; // Ch5

// ====== 元スケッチの変数群 ======
int servo_left_pin  = 6; // D5 = GPIO6
int servo_right_pin = 5; // D4 = GPIO5

volatile int elevator = 0;
volatile int flapamp  = 0;
volatile float delaytime = 100; // フラップ周波数（Ch5から計算）
float elapsed_time = 0;
float dt;
unsigned long current_time, prev_time;

volatile int ch3value = 1000; //Ch3
volatile int ch1value = 1500; //Ch1
volatile int ch2value = 1500; //Ch2
volatile int ch4value = 1500; //Ch4
volatile int ch5value = 1500; //Ch5

static int servo_comm1 = 0;
static int servo_comm2 = 0;
volatile int rudder = 0;
float glide_deg = 0; // グライド時の角度補正
static float servo_zero1 = 0;
static float servo_zero2 = 0;

Servo servo_left, servo_right;

// ====== ESP-NOW受信コールバック（新形式） ======
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  if (len == sizeof(PpmPacket)) {
    PpmPacket recvData;
    memcpy(&recvData, incomingData, sizeof(PpmPacket));

    // ESP-NOWからの生値をグローバルに格納（μsec）
    ch1Value = recvData.ch1;
    ch2Value = recvData.ch2;
    ch3Value = recvData.ch3;
    ch4Value = recvData.ch4;
    ch5Value = recvData.ch5;

    // デバッグしたければコメントアウト解除
    /*
    Serial.print("Recv CH: ");
    Serial.print(ch1Value); Serial.print(' ');
    Serial.print(ch2Value); Serial.print(' ');
    Serial.print(ch3Value); Serial.print(' ');
    Serial.print(ch4Value); Serial.print(' ');
    Serial.println(ch5Value);
    */
  }
}

// ====== セットアップ ======
void setup() {
  Serial.begin(9600);
  delay(1000);

  // サーボピン設定
  pinMode(servo_left_pin, OUTPUT);
  pinMode(servo_right_pin, OUTPUT);

  servo_left.attach(servo_left_pin);
  servo_right.attach(servo_right_pin);

  // RX側ESP-NOW初期化
  WiFi.mode(WIFI_STA);
  delay(200);

  Serial.print("RX My MAC: ");
  Serial.println(WiFi.macAddress());

  esp_err_t ret = esp_now_init();
  Serial.print("esp_now_init result: ");
  Serial.println(ret);
  if (ret != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true) { delay(1000); }
  }

  esp_now_register_recv_cb(OnDataRecv);

  // 起動直後の暴れ防止
  delay(2000);
}

// ====== メインループ ======
void loop() {
  // 時間計測
  prev_time = current_time;
  current_time = micros();
  dt = (current_time - prev_time) / 1000000.0f;
  elapsed_time = elapsed_time + dt;

  // ESP-NOWからの値をローカルにコピーして元コードの変数へ
  uint16_t ch1, ch2, ch3, ch4, ch5;
  noInterrupts();
  ch1 = ch1Value;
  ch2 = ch2Value;
  ch3 = ch3Value;
  ch4 = ch4Value;
  ch5 = ch5Value;
  interrupts();

  ch3value = ch3; // Ch3
  ch1value = ch1; // Ch1
  ch2value = ch2; // Ch2
  ch4value = ch4; // Ch4
  ch5value = ch5; // Ch5

  // ここから先は元のロジックそのまま
  rudder   = (int)(ch1value - 1500);          // Ch1 Aileron
  elevator = (int)(ch2value - 1500);          // Ch2 Elevator
  flapamp  = (int)(ch4value - 1500);          // Ch4 R/L差分
  delaytime = (int)((ch5value - 950) / 5.0f); // Ch5 Flapping frequency

  if (ch3value > 1080) {
    // フラップ動作中
    if (elapsed_time < delaytime / 1000.0f) {
      servo_comm1 = (int)((ch3value - 1000) / 2 + 1500
                          + rudder - elevator + servo_zero1 + flapamp);

      servo_comm2 = (int)(1000
                          + (2000 - ((ch3value - 1000) / 2 + 1500))
                          + rudder + elevator - servo_zero2 + flapamp);

      servo_left.writeMicroseconds(servo_comm1);
      servo_right.writeMicroseconds(servo_comm2);
    }

    if ((elapsed_time > delaytime / 1000.0f) &&
        (elapsed_time < (delaytime + delaytime) / 1000.0f)) {

      servo_comm1 = (int)((ch3value - 1000) / 2 + 1500
                          + rudder + elevator + servo_zero1 - flapamp);

      servo_comm2 = (int)(1000
                          + (2000 - ((ch3value - 1000) / 2 + 1500))
                          + rudder - elevator - servo_zero2 - flapamp);

      // ここは左右を入れ替えた動き
      servo_left.writeMicroseconds(servo_comm2);
      servo_right.writeMicroseconds(servo_comm1);
    }
  } else {
    // グライドモード
    servo_comm1 = (int)(1500 + rudder - elevator + glide_deg);
    servo_comm2 = (int)(1500 + rudder + elevator - glide_deg);

    servo_left.writeMicroseconds(servo_comm1);
    servo_right.writeMicroseconds(servo_comm2);
  }

  // 1フラップ周期が終わったら時間リセット
  if (elapsed_time > (delaytime + delaytime) / 1000.0f) {
    elapsed_time = 0;
  }

  // 必要ならここにデバッグ出力を追加
  /*
  Serial.print("ch1 "); Serial.print(ch1value);
  Serial.print(" ch2 "); Serial.print(ch2value);
  Serial.print(" ch3 "); Serial.print(ch3value);
  Serial.print(" ch4 "); Serial.print(ch4value);
  Serial.print(" ch5 "); Serial.print(ch5value);
  Serial.print(" delaytime "); Serial.print(delaytime);
  Serial.print(" servo1 "); Serial.print(servo_comm1);
  Serial.print(" servo2 "); Serial.println(servo_comm2);
  */
}