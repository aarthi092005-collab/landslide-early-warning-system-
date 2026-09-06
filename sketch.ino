#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <DHT.h>
#include <RTClib.h>

// ================= Wi-Fi =================
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ================= Telegram =================
const String botToken = "YOUR_BOT_TOKEN_HERE";   // <-- unga real token podunga
const String chatId   = "YOUR_CHAT_ID_HERE";

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= RTC (DS1307) =================
RTC_DS1307 rtc;

// ================= DHT22 =================
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ================= Pins =================
const int buzzerPin    = 23;
const int greenLedPin  = 18;
const int yellowLedPin = 17;
const int redLedPin    = 19;
const int ackButtonPin = 15;

// ================= Auto Simulation =================
int simulatedValue = 0;
int step = 150;

// ================= Moving Average (Trend Detection) =================
#define AVG_WINDOW 5
int readingWindow[AVG_WINDOW];
int windowIndex = 0;
int movingAverage = 0;

float humidity = 0;
float temperature = 0;

bool alertAcknowledged = false;
unsigned long lastAlertTime = 0;

String previousStatus = "";
String systemStatus = "SAFE";

// =================================================
String urlEncode(const String &str) {
  String encoded = "";
  char c, code0, code1;
  for (size_t i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum((unsigned char)c)) {
      encoded += c;
    } else if (c == ' ') {
      encoded += "%20";
    } else {
      code1 = (c & 0x0F) + '0';
      if ((c & 0x0F) > 9) code1 = (c & 0x0F) - 10 + 'A';
      c = (c >> 4) & 0x0F;
      code0 = c + '0';
      if (c > 9) code0 = c - 10 + 'A';
      encoded += '%'; encoded += code0; encoded += code1;
    }
  }
  return encoded;
}

void setup() {
  Serial.begin(115200);

  pinMode(buzzerPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(ackButtonPin, INPUT_PULLUP);

  dht.begin();
  delay(2000);  // DHT22 stabilize aaga time kudukrom

  if (!rtc.begin()) {
    Serial.println("RTC not found!");
  }
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // first run only

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("LANDSLIDE EWS");
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");
  delay(1500);
  lcd.clear();

  // Initialize moving average window
  for (int i = 0; i < AVG_WINDOW; i++) {
    readingWindow[i] = 0;
  }
}

void loop() {

  // ---------------- Auto Simulated Soil Value ----------------
  simulatedValue += step;
  if (simulatedValue >= 4095 || simulatedValue <= 0) {
    step = -step;
  }

  // ---------------- Moving Average Calculation ----------------
  readingWindow[windowIndex] = simulatedValue;
  windowIndex = (windowIndex + 1) % AVG_WINDOW;

  long sum = 0;
  for (int i = 0; i < AVG_WINDOW; i++) {
    sum += readingWindow[i];
  }
  movingAverage = sum / AVG_WINDOW;

  // ---------------- DHT22 ----------------
  humidity    = dht.readHumidity();
  temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    delay(200);
    humidity    = dht.readHumidity();
    temperature = dht.readTemperature();
  }

  if (isnan(humidity) || isnan(temperature)) {
    humidity = 0;
    temperature = 0;
  }

  DateTime now = rtc.now();

  // ---------------- Acknowledge Button ----------------
  if (digitalRead(ackButtonPin) == LOW) {
    alertAcknowledged = true;
    noTone(buzzerPin);
    lcd.setCursor(0, 1);
    lcd.print("Alert Silenced ");
    delay(500);
  }

  // ---------------- Serial Debug ----------------
  Serial.print("Value: "); Serial.print(simulatedValue);
  Serial.print(" | Moving Avg: "); Serial.print(movingAverage);
  Serial.print(" | Humidity: "); Serial.print(humidity);
  Serial.print(" | Time: "); Serial.print(now.hour());
  Serial.print(":"); Serial.println(now.minute());

  // ---------------- LCD Line 1 ----------------
  lcd.setCursor(0, 0);
  lcd.print("Avg:");
  lcd.print(movingAverage);
  lcd.print("     ");

  // ---------------- 3-Stage Logic (Using Moving Average) ----------------
  if (movingAverage > 3000) {
    // ---------- DANGER ----------
    systemStatus = "DANGER";
    digitalWrite(redLedPin, HIGH);
    digitalWrite(yellowLedPin, LOW);
    digitalWrite(greenLedPin, LOW);

    if (!alertAcknowledged) {
      tone(buzzerPin, 1500);
    }

    lcd.setCursor(0, 1);
    lcd.print("DANGER! ALERT  ");

    if (millis() - lastAlertTime > 10000) {
      String msg = "🚨  DANGER ALERT - NER 🚨\n"
                   "⚠️  Value: " + String(simulatedValue) + "\n"
                   "📊  Moving Avg: " + String(movingAverage) + "\n"
                   "💧  Humidity: " + String((int)humidity) + "%";
      sendTelegramMessage(msg);
      lastAlertTime = millis();
      alertAcknowledged = false;
    }
  }
  else if (movingAverage >= 2000) {
    // ---------- WARNING ----------
    systemStatus = "WARNING";
    digitalWrite(redLedPin, LOW);
    digitalWrite(yellowLedPin, HIGH);
    digitalWrite(greenLedPin, LOW);
    tone(buzzerPin, 800, 200);

    lcd.setCursor(0, 1);
    lcd.print("WARNING Monitor ");

    if (previousStatus != "WARNING") {
      String msg = "WARNING - NER\n"
                   "Value: " + String(simulatedValue) + "\n"
                   "Moving Avg: " + String(movingAverage) + "\n"
                   "Humidity: " + String((int)humidity) + "%";
      sendTelegramMessage(msg);
    }
  }
  else {
    // ---------- SAFE ----------
    systemStatus = "SAFE";
    digitalWrite(redLedPin, LOW);
    digitalWrite(yellowLedPin, LOW);
    digitalWrite(greenLedPin, HIGH);
    noTone(buzzerPin);

    lcd.setCursor(0, 1);
    lcd.print("Status: SAFE    ");

    if (previousStatus != "SAFE" && previousStatus != "") {
      String msg = "SYSTEM NORMAL - NER\nStatus: SAFE";
      sendTelegramMessage(msg);
    }
    alertAcknowledged = false;
  }

  previousStatus = systemStatus;

  delay(1000);
}

// =================================================
void sendTelegramMessage(String message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected!");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  String url = "https://api.telegram.org/bot" + botToken +
               "/sendMessage?chat_id=" + chatId +
               "&text=" + urlEncode(message);

  Serial.println("Sending Telegram...");

  if (http.begin(client, url)) {
    int code = http.GET();
    Serial.print("Telegram HTTP Code: ");
    Serial.println(code);
    if (code != 200) {
      Serial.println(http.getString());
    }
    http.end();
  }
}
