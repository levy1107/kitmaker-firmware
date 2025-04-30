# auto_commit.py

import os
import openai
from github import Github
from datetime import datetime

# ————— Configuración —————
openai.api_key = os.getenv("OPENAI_API_KEY")
gh         = Github(os.getenv("GITHUB_TOKEN"))
repo       = gh.get_repo("levy1107/kitmaker-firmware")
SKETCH     = "sketches/automatic.ino"
COMMIT_TAG = "🤖 Auto-update"
# ——————————————————————

def fetch_current():
    """
    Descarga el contenido y SHA del sketch actual desde GitHub.
    """
    try:
        f = repo.get_contents(SKETCH)
        return f.decoded_content.decode("utf-8"), f.sha
    except Exception:
        return "", None

def generate_updated_code(current_code: str, user_request: str) -> str:
    """
    Envía a ChatGPT el código actual y la instrucción de cambio,
    usando un prompt que describe todo el hardware de la placa ESP32 KitMaker 2.0.
    """
    system_msg = (
        "Eres un asistente que responde únicamente con código válido para la placa "
        "ESP32 KitMaker 2.0. No agregues texto extra ni explicaciones; devuelve solo "
        "el contenido del archivo .ino.\n\n"
        "La placa ESP32 KitMaker 2.0 tiene los siguientes pines y componentes:\n"
        "  • Microcontrolador: ESP32\n"
        "  • GPIO39: Sensor de luz TEMT6000 (ADC)\n"
        "  • I2C (SDA=GPIO21, SCL=GPIO22): HTU21D (temp/humedad) y OLED 128×64\n"
        "  • GPIO14: Sensor de vibración Tilt BL2500\n"
        "  • GPIO0: Pulsador izquierdo\n"
        "  • GPIO15: Pulsador medio (usado para OTA pull 5 s)\n"
        "  • GPIO13: Pulsador derecho\n"
        "  • GPIO36: Switch medir batería (ADC)\n"
        "  • GPIO27: 4 NeoPixels programables\n"
        "  • GPIO12: Buzzer pasivo\n"
        "  • Puerto MicroUSB CP2102 para carga y programación\n"
        "  • Indicadores LED de carga/RxTx, JST para batería, RJ9, etc.\n\n"
        "Incluye siempre la inicialización de Wi-Fi (SSID: Polotics, P4L4T3cs) "
        "y la lógica de Pull-OTA desde:\n"
        "  https://raw.githubusercontent.com/levy1107/kitmaker-firmware/main/firmware/latest.bin\n"
    )

    user_msg = (
        "Aquí está el código actual:\n```cpp\n"
        f"{current_code}\n```\n\n"
        "Por favor, aplica esta modificación:\n"
        f"{user_request}\n\n"
        "Devuélveme únicamente el archivo .ino completo dentro de un solo bloque de código."
    )

    resp = openai.ChatCompletion.create(
        model="gpt-4-turbo",
        messages=[
            {"role": "system", "content": system_msg},
            {"role": "user",   "content": user_msg}
        ]
    )

    code_block = resp.choices[0].message.content
    # Limpieza de delimitadores
    return code_block.strip().strip("```").replace("cpp\n", "")

def commit_updated_code(new_code: str, sha: str):
    """
    Actualiza (o crea) el sketch en GitHub con el código generado.
    """
    commit_msg = f"{COMMIT_TAG} {datetime.utcnow().isoformat()}"
    if sha:
        repo.update_file(SKETCH, commit_msg, new_code, sha)
    else:
        repo.create_file(SKETCH, commit_msg, new_code)

def main():
    # Verificar credenciales en entorno
    if not openai.api_key or not os.getenv("GITHUB_TOKEN"):
        print("❌ Error: faltan las variables OPENAI_API_KEY o GITHUB_TOKEN.")
        return

    # 1) Obtener código actual
    current_code, sha = fetch_current()
    if not current_code:
        print(f"⚠️ No se encontró {SKETCH}, se creará uno nuevo.")

    # 2) Pedir al usuario la modificación
    user_request = input("¿Qué quieres cambiar en el sketch?\n> ").strip()
    if not user_request:
        print("❌ No se especificó ninguna modificación.")
        return

    # 3) Generar código actualizado
    print("⏳ Generando código actualizado con ChatGPT…")
    updated_code = generate_updated_code(current_code, user_request)

    # 4) Commitear a GitHub
    print("📥 Subiendo el nuevo sketch a GitHub…")
    commit_updated_code(updated_code, sha)
    print("✅ ¡Listo! GitHub Actions compilará y actualizará latest.bin.")

if __name__ == "__main__":
    main()
