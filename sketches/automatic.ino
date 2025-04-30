// sketch: automatic.ino
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_HTU21DF.h>
#include <Adafruit_NeoPixel.h>

// ————— Configuración de hardware —————
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define LDR_PIN         39
#define NEOPIXEL_PIN    27
#define NEOPIXEL_COUNT  4
#define BUTTON_PIN      15
// ————————————————————————————————

// ————— Wi-Fi y URL OTA —————
const char* ssid            = "Polotics";
const char* password        = "P4L4T3cs";
const char* FIRMWARE_URL    =
  "https://raw.githubusercontent.com/levy1107/kitmaker-firmware/main/firmware/latest.bin";
// ——————————————————————————

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_HTU21DF   htu;
Adafruit_NeoPixel  pixels(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Wire.begin();

  if (!htu.begin()) { Serial.println("Error: HTU21D"); while (1); }
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { Serial.println("Error: OLED"); while (1); }
  display.clearDisplay(); display.display();

  pixels.begin();
  for (int i = 0; i < NEOPIXEL_COUNT; i++)
    pixels.setPixelColor(i, pixels.Color(255, 0, 0));
  pixels.show();

  WiFi.begin(ssid, password);
  Serial.print("WiFi…");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" OK");
}

void loop() {
  static unsigned long pressStart = 0;
  static bool checking = false;

  // Lee sensores
  float temp   = htu.readTemperature();
  float hum    = htu.readHumidity();
  int   ldrRaw = analogRead(LDR_PIN);

  // Muestra en OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,  0); display.printf("Temp: %.2f C", temp);
  display.setCursor(0, 10); display.printf("Hum : %.2f %%", hum);
  display.setCursor(0, 20); display.printf("LDR : %4d", ldrRaw);
  display.display();

  // Si el botón PIN15 está presionado 5 s, dispara OTA pull
  if (digitalRead(BUTTON_PIN) == LOW) {
    if (!checking) {
      checking = true;
      pressStart = millis();
    } else if (millis() - pressStart >= 5000) {
      Serial.println("Botón 5s: buscando update...");
      HTTPClient http;
      http.begin(FIRMWARE_URL);
      if (http.GET() == HTTP_CODE_OK) {
        int len = http.getSize();
        if (len > 0 && Update.begin(len)) {
          WiFiClient *stream = http.getStreamPtr();
          size_t written = Update.writeStream(*stream);
          if (written == len && Update.end()) {
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
