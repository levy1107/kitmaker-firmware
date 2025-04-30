# auto_commit.py
import os
import openai
from github import Github
from datetime import datetime

# ————— Configuración —————
openai.api_key = os.getenv("OPENAI_API_KEY")
gh        = Github(os.getenv("GITHUB_TOKEN"))
repo      = gh.get_repo("levy1107/kitmaker-firmware")
SKETCH    = "sketches/automatic.ino"
# ——————————————————————

def fetch_current():
    try:
        f = repo.get_contents(SKETCH)
        return f.decoded_content.decode("utf-8"), f.sha
    except:
        return "", None

def generate(new_req, code):
    sys_msg = ("Eres un asistente que solo devuelve código Arduino válido "
               "en un único bloque, sin texto adicional.")
    user_msg = (f"Este es el código actual:\n```cpp\n{code}\n```\n\n"
                f"Aplica esta modificación: {new_req}\n\n"
                "Devuélveme solo el código completo en un bloque .ino.")
    resp = openai.ChatCompletion.create(
        model="gpt-4-turbo",
        messages=[
          {"role":"system", "content":sys_msg},
          {"role":"user",   "content":user_msg}
        ]
    )
    txt = resp.choices[0].message.content
    return txt.strip().strip("```").replace("cpp\n", "")

def commit(code, sha):
    msg = f"🤖 Auto-update {datetime.utcnow().isoformat()}"
    if sha:
        repo.update_file(SKETCH, msg, code, sha)
    else:
        repo.create_file(SKETCH, msg, code)

if __name__=="__main__":
    if not os.getenv("OPENAI_API_KEY") or not os.getenv("GITHUB_TOKEN"):
        print("❌ Define OPENAI_API_KEY y GITHUB_TOKEN.")
        exit(1)

    current, sha = fetch_current()
    req = input("¿Qué cambio quieres aplicar?\n> ")
    print("⏳ Generando código…")
    updated = generate(req, current)
    print("📥 Subiendo a GitHub…")
    commit(updated, sha)
    print("✅ Listo.")
