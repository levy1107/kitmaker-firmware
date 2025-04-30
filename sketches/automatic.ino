#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_HTU21DF.h>
#include <Adafruit_NeoPixel.h>

// ————— Hardware —————
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define LDR_PIN         39
#define NEOPIXEL_PIN    27
#define NEOPIXEL_COUNT  4
#define BUTTON_PIN      15
// ——————————————————

// ————— Wi-Fi y OTA URL —————
const char* ssid         = "Polotics";
const char* password     = "P4L4T3cs";
const char* FIRMWARE_URL =
  "https://raw.githubusercontent.com/levy1107/kitmaker-firmware/main/firmware/latest.bin";
// ——————————————————————

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_HTU21DF   htu;
Adafruit_NeoPixel  pixels(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Wire.begin();

  // Sensor HTU21D
  if (!htu.begin()) {
    Serial.println("Error: HTU21D");
    while (true);
  }

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Error: OLED");
    while (true);
  }
  display.clearDisplay();
  display.display();

  // NeoPixels (verde)
  pixels.begin();
  for (int i = 0; i < NEOPIXEL_COUNT; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 255, 0));
  }
  pixels.show();

  // Conexión Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a ");
  Serial.print(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" OK");
}

void loop() {
  static unsigned long pressStart = 0;
  static bool checking = false;

  // Leer sensores
  float temp   = htu.readTemperature();
  float hum    = htu.readHumidity();
  int   ldrRaw = analogRead(LDR_PIN);

  // Mostrar en OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,  0); display.printf("T:%.1fC H:%.1f%%", temp, hum);
  display.setCursor(0, 10); display.printf("LDR:%4d", ldrRaw);
  display.display();

  // Si mantienes pulsado BUTTON_PIN 5s haz pull-OTA
  if (digitalRead(BUTTON_PIN) == LOW) {
    if (!checking) {
      checking   = true;
      pressStart = millis();
    } else if (millis() - pressStart >= 5000) {
      Serial.println("OTA pull: buscando update...");
      HTTPClient http;
      http.begin(FIRMWARE_URL);
      if (http.GET() == HTTP_CODE_OK) {
        int len = http.getSize();
        if (len > 0 && Update.begin(len)) {
          WiFiClient *stream = http.getStreamPtr();
          size_t written = Update.writeStream(*stream);
          if (written == len && Update.end()) {
            Serial.println("OTA OK, reiniciando...");
            ESP.restart();
          }
        }
      }
      http.end();
      checking = false;
    }
  } else {
    checking = false;
  }

  delay(2000);
}

