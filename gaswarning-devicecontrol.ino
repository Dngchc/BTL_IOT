#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

const char* ssid = "dangchuc";
const char* password = "dangchuc";
const char* mqtt_server = "broker.hivemq.com";
const char* gasTopic = "BTL/Data";
const char* ledTopic = "BTL/Control";

WiFiClient espClient;
PubSubClient client(espClient);

const int led1Pin = 27;  
const int buttonPin = 32;  
bool manualControl = true;  // Theo dõi xem LED có được điều khiển bằng nút ấn hay không
bool lastLedState = LOW;   // Theo dõi trạng thái LED trước đó

const int mq2Pin = 34;
const int led2Pin = 26;
const int buzzPin = 2;
bool WarningSent = false; // Theo dõi trạng thái gửi thông báo

void setup_wifi() {
  delay(10);
  Serial.println("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println(message);

  if (strcmp(topic, ledTopic) == 0) {
    if (manualControl) {
      if (message == "on") {
        digitalWrite(led1Pin, HIGH);
      } else if (message == "off") {
        digitalWrite(led1Pin, LOW);
      }
    }
    // Đặt lại trạng thái điều khiển bằng nút sau khi nhận được lệnh từ MQTT
    manualControl = true;
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT Broker...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(ledTopic);
      client.subscribe(gasTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(led1Pin, OUTPUT);
  pinMode(buttonPin, INPUT);

  pinMode(mq2Pin, INPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(buzzPin, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(1, 0);
  lcd.print("System Loading");
  for (int a = 0; a <= 15; a++) {
    lcd.setCursor(a, 1);
    lcd.print(".");
    delay(200);
  }
  
  lcd.clear();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Kiểm tra trạng thái nút và điều khiển LED
  if (digitalRead(buttonPin) == HIGH) {
    manualControl = false;  // Nếu nút ấn được giữ, ngăn chặn điều khiển từ MQTT
    if (lastLedState == LOW) {
      digitalWrite(led1Pin, HIGH);
      client.publish(ledTopic, "on");
      lastLedState = HIGH;
    }
  } else {
    if (lastLedState == HIGH) {
      digitalWrite(led1Pin, LOW);
      client.publish(ledTopic, "off");
      lastLedState = LOW;
    }
    manualControl = true;
  }


  int mq2Value = analogRead(mq2Pin);
  mq2Value = map(mq2Value, 0, 4095, 0, 100);
  Serial.print("Gas Value: ");
  Serial.println(mq2Value);
  delay(500);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gas Value: ");
  lcd.print(mq2Value);

  if(mq2Value < 25){
    lcd.setCursor(0, 1);
    lcd.print("Safe ");
  }
  else{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Warning !");
  }

  String mq2Data = String(mq2Value);
  client.publish(gasTopic, mq2Data.c_str());
  if (mq2Value >= 25 && !WarningSent) {
    digitalWrite(led2Pin, HIGH);
    digitalWrite(buzzPin, HIGH);
    client.publish(gasTopic, "warning");
    WarningSent = true; // Đánh dấu đã gửi cảnh báo
  } else if (mq2Value < 25 && WarningSent) {
    digitalWrite(led2Pin, LOW);
    digitalWrite(buzzPin, LOW); 
    WarningSent = false; // Đặt lại trạng thái cảnh báo khi giá trị thấp hơn 500
  }
}



