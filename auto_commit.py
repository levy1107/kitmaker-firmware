#!/usr/bin/env python
# auto_commit.py  –  genera un nuevo .ino, lo sube al repo y deja que
# GitHub Actions construya firmware/latest.bin

import os, re, openai
from github import Github
from datetime import datetime

# ───────── Configuración de tokens ─────────
openai.api_key  = os.getenv("OPENAI_API_KEY")
gh              = Github(os.getenv("GITHUB_TOKEN"))
repo            = gh.get_repo("levy1107/kitmaker-firmware")
SKETCH          = "sketches/automatic.ino"
COMMIT_TAG      = "🤖 Auto‑update"
# ────────────────────────────────────────────


def fetch_current():
    """Descarga contenido y SHA del sketch actual."""
    try:
        f = repo.get_contents(SKETCH)
        return f.decoded_content.decode(), f.sha
    except Exception:
        return "", None


def bump_version(code: str) -> str:
    """
    Inserta o actualiza #define FW_VERSION con marca UTC YYYYMMDDHHMM.
    Cambiar ese string garantiza un binario distinto.
    """
    stamp = datetime.utcnow().strftime("%Y%m%d%H%M")
    if "#define FW_VERSION" in code:
        code = re.sub(r'#define FW_VERSION\s*"[^"]+"',
                      f'#define FW_VERSION "{stamp}"', code)
    else:
        code = '#define FW_VERSION "' + stamp + '"\n' + code
    return code


def generate_updated_code(current_code: str, user_request: str) -> str:
    """Llama a ChatGPT y devuelve el .ino completo actualizado."""
    system_msg = (
        "Eres un asistente que responde EXCLUSIVAMENTE con código .ino "
        "para la placa ESP32 KitMaker 2.0, sin texto extra.\n\n"
        "La placa dispone de:\n"
        "  • ESP32 MCU\n"
        "  • GPIO39  → sensor luz TEMT6000 (ADC)\n"
        "  • I2C     → HTU21D + OLED 128×64 (SDA 21, SCL 22)\n"
        "  • GPIO14  → sensor vibración BL2500\n"
        "  • GPIO0   → pulsador izquierdo\n"
        "  • GPIO15  → pulsador medio (OTA pull 5 s)\n"
        "  • GPIO13  → pulsador derecho\n"
        "  • GPIO36  → medición batería (ADC)\n"
        "  • GPIO27  → 4 NeoPixels\n"
        "  • GPIO12  → buzzer pasivo\n\n"
        "Incluye SIEMPRE en la salida:\n"
        "  • Configuración Wi‑Fi (SSID: Polotics / pass: P4L4T3cs)\n"
        "  • Lógica Pull‑OTA con URL:\n"
        "    https://raw.githubusercontent.com/levy1107/kitmaker-firmware/main/firmware/latest.bin\n"
        "  • Una línea  #define FW_VERSION \"YYYYMMDDHHMM\"  y "
        "    ¡debe incrementarse en cada modificación!\n"
    )

    user_msg = (
        "Código actual:\n```cpp\n" + current_code + "\n```\n\n"
        "Aplica esta modificación:\n" + user_request + "\n\n"
        "Devuélveme SOLO el archivo .ino completo dentro de ```."
    )

    resp = openai.ChatCompletion.create(
        model="gpt-4-turbo",
        messages=[
            {"role": "system", "content": system_msg},
            {"role": "user",   "content": user_msg},
        ],
    )

    code = resp.choices[0].message.content
    code = code.strip().strip("```").replace("cpp\n", "")
    return bump_version(code)


def commit_updated_code(new_code: str, sha: str):
    """Sube el nuevo sketch al repositorio."""
    msg = f"{COMMIT_TAG} {datetime.utcnow().isoformat(timespec='seconds')}Z"
    if sha:
        repo.update_file(SKETCH, msg, new_code, sha)
    else:
        repo.create_file(SKETCH, msg, new_code)


def main():
    if not openai.api_key or not os.getenv("GITHUB_TOKEN"):
        print("❌ Falta OPENAI_API_KEY o GITHUB_TOKEN")
        return

    current_code, sha = fetch_current()
    if not current_code:
        print(f"⚠️ {SKETCH} no existe; se creará.")

    user_request = input("¿Qué quieres cambiar en el sketch?\n> ").strip()
    if not user_request:
        print("Sin cambios pedidos."); return

    print("⏳ Generando código con ChatGPT…")
    updated_code = generate_updated_code(current_code, user_request)

    print("📥 Subiendo a GitHub…")
    commit_updated_code(updated_code, sha)
    print("✅ Listo: GitHub Actions compilará latest.bin.")


if __name__ == "__main__":
    main()
