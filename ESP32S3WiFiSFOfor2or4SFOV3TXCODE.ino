// 260408 Version3 SFO for 2 or 4 servo CODE by K.Kakuta
// 260408 TX16S(Slave mode)- ppm signal --ESP32S3--WiFi(Slave mode)--ESP32S3--2ServoFO
// ESP-NOW ch1〜ch8を使う送る　　TX用  

#include <WiFi.h>
#include <esp_now.h>
#include "esp_wifi.h"   // wifi_tx_info_t 用

// ====== 設定値 ======
const int PPM_PIN      = 8;     // TX16SのPPM信号をつないだピン（D9=GPIO8）
const int CHANNEL_NUM  = 8;     // 使うCH数
const int FRAME_GAP_US = 4000;  // フレーム境界判定（4ms以上のギャップ）
const int MIN_PULSE_US = 800;   // パルス最小
const int MAX_PULSE_US = 2200;  // パルス最大

// ====== 構造体：ch1〜ch8 ======
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

PpmPacket sendData;

// RX側ESP32のMACアドレス（必ずRXのMACに合わせる）
uint8_t peerMac[] = {0x1C, 0xDB, 0xD4, 0x75, 0x64, 0xC8};

// ====== PPMデコード用（ISRから更新） ======
volatile uint16_t ppmChannels[CHANNEL_NUM];
volatile int      ppmChannelIndex = 0;
volatile uint32_t lastEdgeMicros  = 0;

// ====== PPM割り込みハンドラ ======
void IRAM_ATTR onPpmEdge() {
  uint32_t now   = micros();
  uint32_t width = now - lastEdgeMicros;
  lastEdgeMicros = now;

  // フレームギャップとみなしてCHインデックスをリセット
  if (width > FRAME_GAP_US) {
    ppmChannelIndex = 0;
    return;
  }

  // 有効なパルス幅だけチャンネルとして採用
  if (width >= MIN_PULSE_US && width <= MAX_PULSE_US) {
    if (ppmChannelIndex < CHANNEL_NUM) {
      ppmChannels[ppmChannelIndex] = (uint16_t)width;
      ppmChannelIndex++;
    }
  }
}

// ====== ESP-NOW送信コールバック（新形式） ======
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// ====== ESP-NOW初期化 ======
void setupEspNow() {
  WiFi.mode(WIFI_STA);
  delay(200);

  Serial.print("TX My MAC: ");
  Serial.println(WiFi.macAddress());

  esp_err_t ret = esp_now_init();
  Serial.print("esp_now_init result: ");
  Serial.println(ret);
  if (ret != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true) { delay(1000); }
  }

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerMac, 6);
  peerInfo.channel = 0;      // 現在のWiFiチャネルを使用
  peerInfo.encrypt = false;

  ret = esp_now_add_peer(&peerInfo);
  Serial.print("esp_now_add_peer result: ");
  Serial.println(ret);
  if (ret != ESP_OK) {
    Serial.println("Failed to add peer");
    while (true) { delay(1000); }
  }
}

// ====== セットアップ ======
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP-NOW TX with PPM (ch1〜ch8 struct)");

  // 初期値は1500us（ニュートラル）
  for (int i = 0; i < CHANNEL_NUM; i++) {
    ppmChannels[i] = 1500;
  }
  sendData.ch1 = 1500;
  sendData.ch2 = 1500;
  sendData.ch3 = 1500;
  sendData.ch4 = 1500;
  sendData.ch5 = 1500;
  sendData.ch6 = 1500;
  sendData.ch7 = 1500;
  sendData.ch8 = 1500;

  // PPM入力ピン設定
  pinMode(PPM_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PPM_PIN), onPpmEdge, RISING);

  // ESP-NOW初期化
  setupEspNow();
}

// ====== メインループ ======
void loop() {
  static unsigned long lastSend = 0;
  const unsigned long SEND_INTERVAL_MS = 20; // 50Hz

  if (millis() - lastSend >= SEND_INTERVAL_MS) {
    lastSend = millis();

    // 割り込みからの値をローカルにコピーして構造体に詰める
    uint16_t ch[CHANNEL_NUM];
    noInterrupts();
    for (int i = 0; i < CHANNEL_NUM; i++) {
      ch[i] = ppmChannels[i];
    }
    interrupts();

    sendData.ch1 = ch[0];
    sendData.ch2 = ch[1];
    sendData.ch3 = ch[2];
    sendData.ch4 = ch[3];
    sendData.ch5 = ch[4];
    sendData.ch6 = ch[5];
    sendData.ch7 = ch[6];
    sendData.ch8 = ch[7];

    // デバッグ表示
    Serial.print("Send CH: ");
    Serial.print(sendData.ch1); Serial.print(' ');
    Serial.print(sendData.ch2); Serial.print(' ');
    Serial.print(sendData.ch3); Serial.print(' ');
    Serial.print(sendData.ch4); Serial.print(' ');
    Serial.print(sendData.ch5); Serial.print(' ');
    Serial.print(sendData.ch6); Serial.print(' ');
    Serial.print(sendData.ch7); Serial.print(' ');
    Serial.println(sendData.ch8);

    // ESP-NOW送信
    esp_err_t result = esp_now_send(peerMac, (uint8_t*)&sendData, sizeof(sendData));
    Serial.print("esp_now_send result: ");
    Serial.println(result);
  }
}