Para imprimir la palabra "domingo" en una pantalla OLED 128x64 con un ESP32, primero necesitas asegurarte de tener las bibliotecas necesarias instaladas. Una de las bibliotecas más utilizadas es `Adafruit_SSD1306` para OLED y `Adafruit_GFX` para gráficos.

A continuación, te muestro un ejemplo básico de cómo hacer esto:

1. Asegúrate de tener instaladas las bibliotecas necesarias. Puedes instalarlas desde el Gestor de Bibliotecas de Arduino buscando "Adafruit SSD1306" y "Adafruit GFX".

2. Conecta tu pantalla OLED al ESP32. Generalmente, las conexiones son las siguientes (pueden variar según tu pantalla y configuraciones):

   - VCC -> 3.3V
   - GND -> GND
   - SCL -> GPIO 22 (o el pin SCL que estés utilizando)
   - SDA -> GPIO 21 (o el pin SDA que estés utilizando)

3. Usa el siguiente código de ejemplo:

```cpp
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Define el tamaño de la pantalla
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Crea una instancia de la pantalla OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // Cambia -1 por el pin de reset si es necesario

void setup() {
  // Inicia la comunicación serial
  Serial.begin(115200);
  
  // Inicia la pantalla
  if (!display.begin(SSD1306_I2C_ADDRESS, OLED_RESET)) {
    Serial.println(F("No se pudo encontrar la pantalla OLED"));
    while (true); // Detener en caso de error
  }
  
  // Limpia la pantalla
  display.clearDisplay();
  
  // Establece el color del texto
  display.setTextColor(SSD1306_WHITE);
  
  // Establece el tamaño del texto
  display.setTextSize(2); // 1 = normal, 2 = grande
  
  // Mueve el cursor a la posición (x, y)
  display.setCursor(0, 20);
  
  // Imprime "domingo"
  display.println("domingo");
  
  // Actualiza la pantalla para mostrar el texto
  display.display();
}

void loop() {
  // No es necesario hacer nada en el bucle
}
```

### Descripción del código:
- Se incluyen las bibliotecas necesarias para trabajar con la pantalla OLED.
- Se define el tamaño y se crea una instancia de `Adafruit_SSD1306`.
- En el `setup()`, se inicializa la pantalla, se configura el color y tamaño del texto, y se imprime la palabra "domingo".
- El método `display.display()` se llama para actualizar la pantalla y mostrar el texto.

### Notas importantes:
- Asegúrate de utilizar la dirección I2C correcta para tu pantalla OLED. En muchas pantallas, la dirección es `0x3C`, pero puedes verificarlo utilizando un escáner I2C.
- Si no ves nada en tu pantalla, verifica las conexiones y asegúrate de que la pantalla esté alimentada correctamente.