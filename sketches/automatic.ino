#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_HTU21DF.h>
#include <Adafruit_NeoPixel.h>

#define FW_VERSION "202505051350"   // ⇦  ponle fecha‑hora nueva en cada build

// ─── Hardware ──────────────────────────────────────────────
#define SCREEN_WIDTH     128
#define SCREEN_HEIGHT     64
#define OLED_RESET        -1

#define LDR_PIN           39
#define NEOPIXEL_PIN      27
#define NEOPIXEL_COUNT     4

#define OTA_BUTTON_PIN    15   // pulsa 5 s para OTA
#define COLOR_BUTTON_PIN   0   // ciclo de colores

#define BUZZER_PIN        12   // TODO: usa si lo necesitas
// ───────────────────────────────────────────────────────────

// ─── Wi‑Fi & OTA ───────────────────────────────────────────
const char* ssid     = "PoloTics";
const char* password = "P4L4T3cs";

const char* FIRMWARE_URL =
  "https://raw.githubusercontent.com/levy1107/kitmaker-firmware/main/firmware/latest.bin";
// ───────────────────────────────────────────────────────────

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_HTU21DF  htu;
Adafruit_NeoPixel pixels(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void showMessage(const char* l1, const char* l2 = nullptr, uint8_t sz = 2) {
  display.clearDisplay();
  display.setTextSize(sz);
  display.setTextColor(SSD1306_WHITE);
  int y = (SCREEN_HEIGHT - 8 * sz * (l2 ? 2 : 1)) / 2;
  display.setCursor(0, y);           display.print(l1);
  if (l2) { display.setCursor(0, y + 8 * sz); display.print(l2); }
  display.display();
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Firmware " FW_VERSION));

  pinMode(OTA_BUTTON_PIN,    INPUT_PULLUP);
  pinMode(COLOR_BUTTON_PIN,  INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED FAIL"); while (true);
  }
  showMessage("Booting…");

  if (!htu.begin()) { showMessage("HTU21D FAIL"); while (true); }

  pixels.begin();
  pixels.clear();
  pixels.show();

  // animación intermitente al arrancar
  uint32_t bootCol[3] = { pixels.Color(255,255,0),
                          pixels.Color(0,0,255),
                          pixels.Color(255,0,0) };
  for (int i = 0; i < 6; ++i) {
    for (int p = 0; p < NEOPIXEL_COUNT; ++p) pixels.setPixelColor(p, bootCol[i%3]);
    pixels.show(); delay(500);
  }

  showMessage("Wi‑Fi", "conectando");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print('.'); }
  showMessage("Wi‑Fi OK", WiFi.localIP().toString().c_str());
  delay(800);
}

void loop() {
  /* ───────── Color button (GPIO0) ───────── */
  static int lastColorState = HIGH, pressCnt = 0;
  int reading = digitalRead(COLOR_BUTTON_PIN);
  if (reading == LOW && lastColorState == HIGH) {
      pressCnt = (pressCnt + 1) % 5;
      uint32_t col[5] = { pixels.Color(255,0,0), pixels.Color(0,255,0),
                          pixels.Color(0,0,255), pixels.Color(255,255,0),
                          pixels.Color(0,0,0) };
      for (int i = 0; i < NEOPIXEL_COUNT; ++i) pixels.setPixelColor(i, col[pressCnt]);
      pixels.show();
  }
  lastColorState = reading;

  /* ───────── Sensores y OLED ───────── */
  float t = htu.readTemperature();
  int   ldr = analogRead(LDR_PIN);
  int   ldrPct = map(ldr, 0, 4095, 0, 100);

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);  display.printf("T:%.1fC", t);
  display.setCursor(0, 32); display.printf("L:%d%%", ldrPct);
  display.display();

  /* ───────── OTA button (GPIO15) ───────── */
  static bool checking = false;
  static unsigned long tStart = 0;

  if (digitalRead(OTA_BUTTON_PIN) == LOW) {
    if (!checking) { checking = true; tStart = millis(); }
    else if (millis() - tStart >= 5000) {
      showMessage("OTA", "buscando…");
      HTTPClient http;
      http.begin(FIRMWARE_URL);
      int code = http.GET();
      Serial.printf("HTTP code: %d\n", code);
      if (code == HTTP_CODE_OK) {
        int len = http.getSize();
        if (len > 0 && Update.begin(len)) {
          WiFiClient *s = http.getStreamPtr();
          size_t wr = Update.writeStream(*s);
          if (wr == len && Update.end()) {
            showMessage("OTA OK", "Reiniciando");
            delay(800); ESP.restart();
          }
        }
      } else showMessage("Sin nueva", "version");
      http.end(); checking = false;
    }
  } else { checking = false; }

  delay(200);
}
