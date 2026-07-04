#define BLYNK_TEMPLATE_ID "TMPL6Y5DoUVbi"
#define BLYNK_TEMPLATE_NAME "Kendali Kipas Otomatis"
#define BLYNK_AUTH_TOKEN "hY5Ql71_Upt6lSZCt3ej6Cg3PTnv6bWR"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

const char *WIFI_SSID = "Wokwi-GUEST";
const char *WIFI_PASSWORD = "";
const int WIFI_CHANNEL = 6;

const byte DHT_PIN = 15;
const byte DHT_TYPE = DHT22;
const byte HEAT_LOAD_PIN = 34;
const byte COOLING_RELAY_PIN = 13;
const byte BUZZER_PIN = 32;
const byte NORMAL_LED_PIN = 25;
const byte WARNING_LED_PIN = 26;
const byte DANGER_LED_PIN = 27;

const int DEFAULT_TEMPERATURE_THRESHOLD = 30;
const int DANGER_TEMPERATURE_THRESHOLD = 40;
const int HEAT_LOAD_THRESHOLD = 2500;
const unsigned long SENSOR_INTERVAL_MS = 2000;
const unsigned long LCD_INTERVAL_MS = 1000;
const unsigned long ALERT_COOLDOWN_MS = 30000;
const int BUZZER_FREQUENCY_HZ = 1000;
const byte BUZZER_CHANNEL = 0;
const byte BUZZER_RESOLUTION_BITS = 8;

#define VPIN_TEMPERATURE V1
#define VPIN_HUMIDITY V2
#define VPIN_HEAT_LOAD V3
#define VPIN_COOLING_STATUS V5
#define VPIN_NORMAL_STATUS V6
#define VPIN_WARNING_STATUS V7
#define VPIN_SYSTEM_STATUS V8
#define VPIN_TEMPERATURE_THRESHOLD V9
#define VPIN_BUZZER_ENABLE V10

DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
BlynkTimer timer;

int temperatureThreshold = DEFAULT_TEMPERATURE_THRESHOLD;
bool buzzerEnabled = true;
unsigned long lastAlertMillis = 0;
String currentStatus = "BOOTING";
float lastTemperature = 0.0;
float lastHumidity = 0.0;
int lastHeatLoad = 0;

BLYNK_CONNECTED()
{
  Blynk.syncVirtual(VPIN_TEMPERATURE_THRESHOLD, VPIN_BUZZER_ENABLE);
}

BLYNK_WRITE(VPIN_TEMPERATURE_THRESHOLD)
{
  const int requestedThreshold = param.asInt();
  temperatureThreshold = constrain(requestedThreshold, 20, 60);
}

BLYNK_WRITE(VPIN_BUZZER_ENABLE)
{
  buzzerEnabled = param.asInt() == 1;
  if (!buzzerEnabled)
  {
    ledcWriteTone(BUZZER_PIN, 0);
  }
}

void connectWifi()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Kendali Kipas");
  lcd.setCursor(0, 1);
  lcd.print("WiFi connect...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(250);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected, IP: ");
  Serial.println(WiFi.localIP());
}

void connectBlynk()
{
  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect(5000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Blynk:");
  lcd.setCursor(7, 0);
  lcd.print(Blynk.connected() ? "Online" : "Offline");
  lcd.setCursor(0, 1);
  lcd.print("System ready");
  delay(1200);
}

String calculateStatus(float temperature, int heatLoad)
{
  const bool temperatureDanger = temperature >= DANGER_TEMPERATURE_THRESHOLD;
  const bool heatDanger = heatLoad >= HEAT_LOAD_THRESHOLD;

  if (temperatureDanger || heatDanger)
  {
    return "DANGER";
  }

  if (temperature >= temperatureThreshold)
  {
    return "WARNING";
  }

  return "NORMAL";
}

void setOutputs(const String &status)
{
  const bool isDanger = status == "DANGER";
  const bool isWarning = status == "WARNING";
  const bool coolingActive = isDanger || isWarning;

  digitalWrite(COOLING_RELAY_PIN, coolingActive ? HIGH : LOW);
  digitalWrite(NORMAL_LED_PIN, status == "NORMAL" ? HIGH : LOW);
  digitalWrite(WARNING_LED_PIN, isWarning ? HIGH : LOW);
  digitalWrite(DANGER_LED_PIN, isDanger ? HIGH : LOW);

  if (isDanger && buzzerEnabled)
  {
    ledcWriteTone(BUZZER_PIN, BUZZER_FREQUENCY_HZ);
    return;
  }

  ledcWriteTone(BUZZER_PIN, 0);
}

void publishToBlynk(const String &status)
{
  const bool coolingActive = status == "WARNING" || status == "DANGER";

  Blynk.virtualWrite(VPIN_TEMPERATURE, lastTemperature);
  Blynk.virtualWrite(VPIN_HUMIDITY, lastHumidity);
  Blynk.virtualWrite(VPIN_HEAT_LOAD, lastHeatLoad);
  Blynk.virtualWrite(VPIN_COOLING_STATUS, coolingActive ? 1 : 0);
  Blynk.virtualWrite(VPIN_NORMAL_STATUS, status == "NORMAL" ? 1 : 0);
  Blynk.virtualWrite(VPIN_WARNING_STATUS, status == "WARNING" ? 1 : 0);
  Blynk.virtualWrite(VPIN_SYSTEM_STATUS, status);
}

void sendDangerEvent(const String &status)
{
  if (status != "DANGER")
  {
    return;
  }

  const unsigned long now = millis();
  if (now - lastAlertMillis < ALERT_COOLDOWN_MS)
  {
    return;
  }

  lastAlertMillis = now;
  Blynk.logEvent("fan_danger_alert", "Suhu ruangan bahaya, kipas pendingin aktif");
}

void readSensors()
{
  const float temperature = dht.readTemperature();
  const float humidity = dht.readHumidity();

  if (!isnan(temperature))
  {
    lastTemperature = temperature;
  }
  if (!isnan(humidity))
  {
    lastHumidity = humidity;
  }

  lastHeatLoad = analogRead(HEAT_LOAD_PIN);
  currentStatus = calculateStatus(lastTemperature, lastHeatLoad);

  setOutputs(currentStatus);
  publishToBlynk(currentStatus);
  sendDangerEvent(currentStatus);

  Serial.print("Suhu: ");
  Serial.print(lastTemperature);
  Serial.print(" C | Kelembapan: ");
  Serial.print(lastHumidity);
  Serial.print(" % | Beban Panas: ");
  Serial.print(lastHeatLoad);
  Serial.print(" | Ambang Batas: ");
  Serial.print(temperatureThreshold);
  Serial.print(" C | Status: ");
  Serial.println(currentStatus);
}

void updateLcd()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(lastTemperature, 1);
  lcd.print("C H:");
  lcd.print(lastHumidity, 0);
  lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print(currentStatus);
  lcd.print(" R:");
  lcd.print(digitalRead(COOLING_RELAY_PIN) == HIGH ? "ON " : "OFF");
}

void setup()
{
  Serial.begin(115200);
  dht.begin();

  pinMode(HEAT_LOAD_PIN, INPUT);
  pinMode(COOLING_RELAY_PIN, OUTPUT);
  pinMode(NORMAL_LED_PIN, OUTPUT);
  pinMode(WARNING_LED_PIN, OUTPUT);
  pinMode(DANGER_LED_PIN, OUTPUT);

  digitalWrite(COOLING_RELAY_PIN, LOW);
  digitalWrite(NORMAL_LED_PIN, LOW);
  digitalWrite(WARNING_LED_PIN, LOW);
  digitalWrite(DANGER_LED_PIN, LOW);

  ledcAttach(BUZZER_PIN, BUZZER_FREQUENCY_HZ, BUZZER_RESOLUTION_BITS);
  ledcWriteTone(BUZZER_PIN, 0);

  lcd.init();
  lcd.backlight();

  connectWifi();
  connectBlynk();

  timer.setInterval(SENSOR_INTERVAL_MS, readSensors);
  timer.setInterval(LCD_INTERVAL_MS, updateLcd);
  readSensors();
  updateLcd();
}

void loop()
{
  Blynk.run();
  timer.run();
}