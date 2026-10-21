#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// 3 phân khu kho: PIR phát hiện xe nâng -> relay đóng ngắt dải đèn của khu đó.
// Trạng thái/lệnh đi qua MQTT: Wokwi <-> broker <-> dashboard.html (hoặc Node-RED/Blynk).
const char* WIFI_SSID = "Wokwi-GUEST";  // WiFi mô phỏng của Wokwi, không mật khẩu
const char* MQTT_HOST = "broker.hivemq.com";
const char* PREFIX    = "huit-khobai-nht";  // đổi nếu trùng người khác trên broker công cộng
                                            // (nhớ đổi cùng giá trị trong dashboard.html)
const uint8_t PIR[3]   = {13, 14, 27};
const uint8_t RELAY[3] = {25, 26, 33};

const unsigned long HOLD_MS = 5000;  // giữ đèn sáng sau khi xe nâng rời khu

enum Mode : uint8_t { AUTO, FORCE_ON, FORCE_OFF };
const char* const MODE_TXT[3] = {"AUTO", "ON", "OFF"};

WiFiClient net;
PubSubClient mqtt(net);
unsigned long lastSeen[3];
Mode mode[3] = {AUTO, AUTO, AUTO};
int8_t lampOn[3] = {-1, -1, -1};  // -1: chưa publish lần nào
int8_t pirOn[3]  = {-1, -1, -1};
unsigned long nextTry = 0;

void pub(uint8_t zone, const char* leaf, const char* val) {
  char t[64];
  snprintf(t, sizeof t, "%s/khu%u/%s", PREFIX, zone + 1, leaf);
  mqtt.publish(t, val, true);  // retained: dashboard mở sau vẫn thấy trạng thái hiện tại
}

void onMessage(char* topic, byte* payload, unsigned int len) {
  // topic: PREFIX/khuN/lenh
  uint8_t pos = strlen(PREFIX) + 4;
  if (topic[pos] < '1' || topic[pos] > '3') return;
  uint8_t z = topic[pos] - '1';
  char msg[8] = {0};
  memcpy(msg, payload, min(len, 7u));
  for (uint8_t m = 0; m < 3; m++)
    if (!strcmp(msg, MODE_TXT[m])) {
      mode[z] = (Mode)m;
      pub(z, "chedo", MODE_TXT[m]);
      Serial.printf("Khu %u: che do %s\n", z + 1, msg);
    }
}

void connectMqtt() {  // không chặn: mất mạng vẫn điều khiển đèn bình thường
  if (mqtt.connected() || WiFi.status() != WL_CONNECTED || millis() < nextTry) return;
  nextTry = millis() + 5000;
  char will[48], id[32];
  snprintf(will, sizeof will, "%s/trangthai", PREFIX);
  snprintf(id, sizeof id, "%s-esp32-%04x", PREFIX, (uint16_t)ESP.getEfuseMac());
  if (!mqtt.connect(id, nullptr, nullptr, will, 0, true, "offline")) return;
  mqtt.publish(will, "online", true);
  char sub[48];
  snprintf(sub, sizeof sub, "%s/+/lenh", PREFIX);
  mqtt.subscribe(sub);
  for (uint8_t i = 0; i < 3; i++) { lampOn[i] = pirOn[i] = -1; pub(i, "chedo", MODE_TXT[mode[i]]); }
  Serial.println(F("MQTT da ket noi"));
}

void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < 3; i++) {
    pinMode(PIR[i], INPUT);
    pinMode(RELAY[i], OUTPUT);
    lastSeen[i] = millis() - HOLD_MS;  // lúc khởi động: không có xe nâng
  }
  WiFi.begin(WIFI_SSID, "", 6);
  mqtt.setServer(MQTT_HOST, 1883);
  mqtt.setCallback(onMessage);
  Serial.println(F("He thong chieu sang kho bai - san sang"));
}

void loop() {
  connectMqtt();
  mqtt.loop();
  unsigned long now = millis();
  for (uint8_t i = 0; i < 3; i++) {
    bool motion = digitalRead(PIR[i]) == HIGH;
    if (motion) lastSeen[i] = now;
    bool want = mode[i] == FORCE_ON || (mode[i] == AUTO && now - lastSeen[i] < HOLD_MS);
    digitalWrite(RELAY[i], want);
    if (motion != pirOn[i]) {
      pirOn[i] = motion;
      pub(i, "pir", motion ? "1" : "0");
    }
    if (want != lampOn[i]) {
      lampOn[i] = want;
      pub(i, "den", want ? "ON" : "OFF");
      Serial.printf("Khu %u: %s\n", i + 1, want ? "SANG" : "TAT");
    }
  }
}
