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

// ————— Wi-Fi & OTA —————
const char* ssid         = "PoloTics";
const char* password     = "P4L4T3cs";
const char* FIRMWARE_URL =
  "https://raw.githubusercontent.com/levy1107/kitmaker-firmware/main/firmware/latest.bin";
// ——————————————————————

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_HTU21DF   htu;
Adafruit_NeoPixel  pixels(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void showMessage(const char* line1, const char* line2 = nullptr, int textSize = 2) {
  display.clearDisplay();
  display.setTextSize(textSize);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, (SCREEN_HEIGHT - 8 * textSize * (line2?2:1)) / 2);
  display.print(line1);
  if (line2) {
    display.setCursor(0, (SCREEN_HEIGHT - 8 * textSize * 2) / 2 + 8 * textSize);
    display.print(line2);
  }
  display.display();
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Wire.begin();

  // Init OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED FAILED");
    while (1);
  }
  showMessage("Iniciando...", nullptr, 2);
  delay(1000);

  // Init HTU21D
  if (!htu.begin()) {
    showMessage("Error HTU21D");
    while (1);
  }

  // Init NeoPixels (azul)
  pixels.begin();
  for (int i = 0; i < NEOPIXEL_COUNT; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 255)); // Azul
  }
  pixels.show();

  // Conecta Wi-Fi
  showMessage("Conectando", "a Wi-Fi", 2);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  showMessage("Wi-Fi OK", WiFi.localIP().toString().c_str(), 2);
  delay(1000);
}

void loop() {
  static unsigned long pressStart = 0;
  static bool checking = false;

  // Leer sensores
  float temp   = htu.readTemperature();
  float hum    = htu.readHumidity();
  int   ldrRaw = analogRead(LDR_PIN);

  // Mostrar datos en OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);   display.printf("T: %.1f C", temp);
  display.setCursor(0, 10);  display.printf("H: %.1f %%", hum);
  display.setCursor(0, 20);  display.printf("LDR: %4d", ldrRaw);
  display.display();

  // Detección de botón largo para OTA pull
  if (digitalRead(BUTTON_PIN) == LOW) {
    if (!checking) {
      checking   = true;
      pressStart = millis();
    } else if (millis() - pressStart >= 5000) {
      // Feedback pantalla
      showMessage("OTA Update", "Buscando...", 2);
      Serial.println("OTA pull: buscando update...");

      HTTPClient http;
      http.begin(FIRMWARE_URL);
      if (http.GET() == HTTP_CODE_OK) {
        int len = http.getSize();
        if (len > 0 && Update.begin(len)) {
          WiFiClient *stream = http.getStreamPtr();
          size_t written = Update.writeStream(*stream);
          if (written == len && Update.end()) {
            showMessage("OTA OK", "Reiniciando", 2);
            delay(1000);
            ESP.restart();
          }
        }
      } else {
        showMessage("No hay", "actualización", 2);
        delay(1000);
      }
      http.end();
      checking = false;
    }
  } else {
    checking = false;
  }

  delay(2000);
}
