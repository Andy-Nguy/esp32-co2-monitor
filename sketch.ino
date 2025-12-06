#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal.h>  // Đổi từ I2C sang parallel

/* ================== CẤU HÌNH CƠ BẢN ================== */

// Nếu chỉ muốn chạy thử trên Wokwi (không cần MQTT cho nhẹ)
// thì COMMENT dòng dưới lại:
#define USE_MQTT

// WiFi (đi thực tế thì đổi SSID/PASS, Wokwi có thể dùng WiFi "Wokwi-GUEST")
// const char* ssid     = "Wokwi-GUEST";
// const char* password = "";

// MQTT public broker (HiveMQ)
const char* mqtt_server = "broker.hivemq.com";
const int   mqtt_port   = 1883;

// Các topic MQTT (Node-RED / Blynk subscribe)
const char* topic_co2_ppm    = "car/co2/ppm";
const char* topic_co2_status = "car/co2/status";

/* ================== KHAI BÁO CHÂN ================== */

#define POT_CO2_PIN      34   // Potentiometer giả lập CO2
#define FAN_LED_PIN      25   // LED xanh tượng trưng quạt
#define ALARM_LED_PIN    26   // LED đỏ khi CO2 cao
#define BUZZER_PIN       27   // Loa cảnh báo (buzzer)

// LCD Parallel 4-bit: RS, E, D4, D5, D6, D7
// Theo diagram.json đã sửa
LiquidCrystal lcd(22, 21, 16, 17, 18, 19);

/* ================== BIẾN TOÀN CỤC ================== */

#ifdef USE_MQTT
WiFiClient espClient;
PubSubClient mqttClient(espClient);
unsigned long lastPublish = 0;
const unsigned long publishInterval = 2000; // ms
#endif

// Ngưỡng giả lập CO2 (ppm)
const float WARNING_LEVEL = 1500.0; // Cảnh báo
const float ALARM_LEVEL   = 3000.0; // Báo động

// Biến cho buzzer beep
unsigned long lastBeepTime = 0;
bool beepState = false;
const unsigned long beepInterval = 500; // ms

/* ================== HÀM WIFI + MQTT ================== */

#ifdef USE_MQTT
void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi failed, vẫn chạy local không MQTT.");
  }
}

void reconnectMQTT() {
  if (WiFi.status() != WL_CONNECTED) {
    return; // Không có WiFi thì khỏi cố MQTT
  }

  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32_CO2_";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      // Có thể subscribe thêm topic điều khiển nếu muốn sau này
      // mqttClient.subscribe("car/co2/cmd");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
      break; // Không loop vô hạn, để CPU còn chạy việc khác
    }
  }
}
#endif

/* ================== LOGIC ĐỌC CO2 + ĐIỀU KHIỂN ================== */

void handleCo2Logic() {
  // Đọc giá trị analog từ potentiometer
  int analogValue = analogRead(POT_CO2_PIN);

  // Giả lập ppm: map 0-4095 về 0-5000 ppm
  float co2ppm = (analogValue * 5000.0) / 4095.0;

  // Xác định trạng thái
  // 0 = OK, 1 = WARNING, 2 = ALARM
  uint8_t state = 0;
  if (co2ppm >= WARNING_LEVEL && co2ppm < ALARM_LEVEL) {
    state = 1;
  } else if (co2ppm >= ALARM_LEVEL) {
    state = 2;
  }

  // Điều khiển phần cứng
  // Fan: bật từ WARNING trở lên
  if (state >= 1) {
    digitalWrite(FAN_LED_PIN, HIGH);
  } else {
    digitalWrite(FAN_LED_PIN, LOW);
  }

  // LED đỏ + buzzer: chỉ bật khi ALARM
  if (state == 2) {
    digitalWrite(ALARM_LED_PIN, HIGH);
    
    // Buzzer beep pattern (500ms on/off)
    unsigned long currentTime = millis();
    if (currentTime - lastBeepTime >= beepInterval) {
      lastBeepTime = currentTime;
      beepState = !beepState;
      digitalWrite(BUZZER_PIN, beepState ? HIGH : LOW);
    }
  } else {
    digitalWrite(ALARM_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    beepState = false;
  }

  // Hiển thị LCD
  lcd.setCursor(0, 0);
  lcd.print("CO2:");
  lcd.print((int)co2ppm);
  lcd.print("ppm      "); // xóa phần dư

  lcd.setCursor(0, 1);
  if (state == 0) {
    lcd.print("Status: OK     ");
  } else if (state == 1) {
    lcd.print("Status: WARN   ");
  } else {
    lcd.print("Status: ALARM! ");
  }

  // Debug Serial
  Serial.print("CO2: ");
  Serial.print(co2ppm);
  Serial.print(" ppm, state=");
  Serial.println(state);

#ifdef USE_MQTT
  // Gửi MQTT
  if (mqttClient.connected()) {
    unsigned long now = millis();
    if (now - lastPublish > publishInterval) {
      lastPublish = now;

      // Gửi ppm
      String ppmStr = String((int)co2ppm);
      mqttClient.publish(topic_co2_ppm, ppmStr.c_str());

      // Gửi status
      String statusStr;
      if (state == 0) statusStr = "OK";
      else if (state == 1) statusStr = "WARNING";
      else statusStr = "ALARM";
      
      mqttClient.publish(topic_co2_status, statusStr.c_str());

      Serial.println("Published to MQTT");
    }
  }
#endif
}

/* ================== SETUP ================== */

void setup() {
  Serial.begin(115200);
  Serial.println("=== ESP32 CO2 Monitor Starting ===");

  // Khởi tạo các chân OUTPUT
  pinMode(FAN_LED_PIN, OUTPUT);
  pinMode(ALARM_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Tắt hết ban đầu
  digitalWrite(FAN_LED_PIN, LOW);
  digitalWrite(ALARM_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Khởi tạo LCD 16x2
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CO2 Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(2000);
  lcd.clear();

#ifdef USE_MQTT
  // Kết nối WiFi
  setupWiFi();

  // Cấu hình MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  // mqttClient.setCallback(mqttCallback); // Nếu có subscribe
#endif

  Serial.println("Setup complete!");
}

/* ================== LOOP ================== */

void loop() {
#ifdef USE_MQTT
  // Giữ kết nối MQTT
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      reconnectMQTT();
    }
    mqttClient.loop();
  }
#endif

  // Xử lý logic CO2
  handleCo2Logic();

  // Delay nhỏ để không spam quá
  delay(100);
}