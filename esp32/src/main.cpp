#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// 3 khu kho: PIR = phát hiện xe nâng, relay = dải đèn LED của khu đó.
// MQTT (xem node-red/flows.json):
//   pub kho/zone/{A,B,C}  {"pir":0|1,"on":0|1}   (retained)
//   pub kho/status        online|offline         (retained, offline = LWT)
//   sub kho/cmd/mode      auto|on|off
const char* WIFI_SSID = "TEN_WIFI";     // TODO: sửa
const char* WIFI_PASS = "MAT_KHAU";     // TODO: sửa
const char* MQTT_HOST = "192.168.1.10"; // TODO: IP máy chạy broker + Node-RED
const uint16_t MQTT_PORT = 1883;

const uint8_t SENSOR[3] = {34, 35, 32}; // chỉ-input, PIR cấp 5V, OUT 3.3V
const uint8_t RELAY[3]  = {25, 26, 27};
const bool RELAY_ACTIVE_LOW = true;     // đa số module relay kích mức thấp
const char ZONE[3] = {'A', 'B', 'C'};
const unsigned long HOLD_MS = 5000;     // giữ đèn sau khi xe nâng rời khu

enum Mode { AUTO, FORCE_ON, FORCE_OFF } mode = AUTO;
unsigned long lastSeen[3];
int8_t lastPir[3] = {-1, -1, -1}, lastOn[3] = {-1, -1, -1};
unsigned long lastTry = 0;

WiFiClient net;
PubSubClient mqtt(net);

void onMessage(char*, byte* p, unsigned int n) {
  String s;
  for (unsigned int i = 0; i < n; i++) s += (char)p[i];
  mode = s == "on" ? FORCE_ON : s == "off" ? FORCE_OFF : AUTO;
  lastOn[0] = lastOn[1] = lastOn[2] = -1;  // báo lại trạng thái ngay
}

void connectMqtt() {
  if (WiFi.status() != WL_CONNECTED || millis() - lastTry < 5000) return;
  lastTry = millis();
  if (!mqtt.connect("kho-esp32", nullptr, nullptr, "kho/status", 1, true, "offline")) return;
  mqtt.publish("kho/status", "online", true);
  mqtt.subscribe("kho/cmd/mode", 1);
  lastPir[0] = lastPir[1] = lastPir[2] = -1;
}

void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < 3; i++) {
    pinMode(SENSOR[i], INPUT);
    pinMode(RELAY[i], OUTPUT);
    digitalWrite(RELAY[i], RELAY_ACTIVE_LOW);  // tắt
    lastSeen[i] = millis() - HOLD_MS;
  }
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
}

void loop() {
  connectMqtt();
  mqtt.loop();
  unsigned long now = millis();
  for (uint8_t i = 0; i < 3; i++) {
    bool pir = digitalRead(SENSOR[i]) == HIGH;
    if (pir) lastSeen[i] = now;
    bool on = mode == FORCE_ON || (mode == AUTO && now - lastSeen[i] < HOLD_MS);
    digitalWrite(RELAY[i], on != RELAY_ACTIVE_LOW);  // đèn chạy kể cả khi mất mạng
    if (pir != lastPir[i] || on != lastOn[i]) {
      char topic[] = "kho/zone/X", msg[24];
      topic[9] = ZONE[i];
      snprintf(msg, sizeof msg, "{\"pir\":%d,\"on\":%d}", pir, on);
      if (mqtt.connected() && mqtt.publish(topic, msg, true)) {
        lastPir[i] = pir;
        lastOn[i] = on;
        Serial.printf("%s %s\n", topic, msg);
      }
    }
  }
}
