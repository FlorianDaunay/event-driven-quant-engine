import requests
from config import GITHUB_KEY, GOOGLE_AI_KEY, COMMIT_HASH, COMMIT_MSG
import base64
from google import genai
import time
import os

GITHUB_OWNER_NAME = "FlorianDaunay"
GITHUB_REPO_NAME = "event-driven-quant-engine"

os.makedirs("docs", exist_ok=True)

OUTPUT_FILENAME = f"docs/{COMMIT_HASH}.md"


GITHUB_HEADERS = {
    "Authorization": f"Bearer {GITHUB_KEY}",
    "Accept": "application/vnd.github+json",
    "Content-Type": "application/json"
}

GOOGLE_CLIENT = genai.Client(
    api_key=GOOGLE_AI_KEY
)
GOOGLE_MODEL = "gemini-3.1-flash-lite"

summary_prompt_func = lambda filepath, file_content: f"""
Tu es un assistant technique. Analyse le fichier suivant et propose un résumé en 2 ou 3 phrases maximum expliquant son rôle dans l'application.
Nom du fichier : { filepath }
Contenu :
{ file_content }
"""

to_markdown_prompt_func = lambda aggregate_summary: f"""
Rôle: Tech Lead & Architecte Logiciel Senior.
Mission: Génère le `README.md` de production pour le dépôt `{GITHUB_REPO_NAME}` en transformant les résumés techniques suivants en une documentation hautement visuelle, structurée et orientée architecture :

{ aggregate_summary }

Contraintes Strictes :
- Démarre directement au premier `#`. Aucun texte d'introduction ni de conclusion en dehors du Markdown généré.
- Privilégie TOUJOURS les schémas (Mermaid), tableaux, et listes structurées plutôt que de longs blocs de texte.
- Style direct, technique, précis et orienté "Clean Architecture". Évite le jargon marketing et les répétitions.

Structure obligatoire à générer :

# {GITHUB_REPO_NAME}

## 🎯 Vue d'ensemble & Problématique
*(Un paragraphe percutant de 3-4 lignes max expliquant le but principal, le problème résolu et la proposition de valeur du projet.)*

## 🛠️ Stack Technique
*(Présente les technologies sous forme de tableau Markdown pour une lecture rapide)*
| Catégorie | Technologie / Framework / Pattern | Rôle dans le projet |
| :--- | :--- | :--- |
| **Langage & Core** | Core tech | ... |
| **Frameworks / Libs** | Libs principales | ... |
| **Architecture** | Patterns (ex: DDD, Clean Archi, MVC...) | ... |

## 🏗️ Architecture & Flux de Données
*(Fournis d'abord un schéma Mermaid `graph TD` ou `sequenceDiagram` montrant l'interaction entre les composants clés ou le flux de données depuis le point d'entrée. Ajoute ensuite 2-3 puces explicatives si nécessaire.)*

```mermaid
// Génère ici le schéma Mermaid adapté
```

## ⚙️ Composants Principaux & Modules
*(Ne liste pas tous les fichiers un par un. Regroupe-les par modules logiques ou dossiers importants sous forme de tableau ou de liste avec blockquotes de type [!NOTE] ou [!IMPORTANT] pour les composants critiques).*

| Module / Dossier | Rôle Macro & Responsabilité | Composants Clés |
| :--- | :--- | :--- |
| `src/core/` | Logique métier et entités | ... |

## 🚀 Guide de Démarrage Rapide

> [!NOTE]
> *(Ajoute un blockquote si un prérequis système spécifique est nécessaire)*

### 🛠️ Prérequis
- Liste des prérequis (ex: Node.js v20+, Docker, Python 3.11...)

### 💻 Installation & Exécution
```bash
# 1. Cloner le projet
git clone [https://github.com/.../](https://github.com/.../){GITHUB_REPO_NAME}.git
cd {GITHUB_REPO_NAME}

# 2. Installer les dépendances
# (Génère la commande exacte selon la stack)

# 3. Configurer l'environnement
# (Indique si un fichier .env est requis)

# 4. Lancer l'application
# (Génère la commande de run/dev standard)
```
"""

def main():

    github_project_tree_files = http_request()
    allowed_filepath_list = filter_files(github_project_tree_files)
    content_file_list = github_get_file(allowed_filepath_list)
    summary_files = call_llm(content_file_list, summary_prompt_func)
    summary_project = aggregate(summary_files)
    generated_markdown = call_llm(summary_project, to_markdown_prompt_func)

    # replace README.md
    save_markdown_in_file(generated_markdown, "README.md")

    # in docs/
    generated_markdown = f"""# Documentation du commit [{COMMIT_HASH}]

> **Message du commit d'origine :** {COMMIT_MSG}

{generated_markdown}
"""
    save_markdown_in_file(generated_markdown, OUTPUT_FILENAME)

def http_request():

    response: requests.Response = requests.get(
        f"https://api.github.com/repos/{GITHUB_OWNER_NAME}/{GITHUB_REPO_NAME}/git/trees/main?recursive=1",
        headers=GITHUB_HEADERS
    ).json()
    return response['tree']

def filter_files(file_list):
    output = []
    EXCLUDED_DIRS = ['node_modules', '.git', '__pycache__', 'dist', 'build', 'public']
    ALLOWED_EXTENSIONS = ['.js', '.ts', '.tsx', '.jsx', '.java', '.sql', '.py']

    for _file in file_list:
        filepath = _file['path']

        if _file['type'] == "tree": continue

        filepath = filepath.replace("\\", "/")
        folders = filepath.split("/")
        if any(folder in EXCLUDED_DIRS for folder in folders): continue

        extension = "." + filepath.split(".")[-1]
        if extension not in ALLOWED_EXTENSIONS: continue

        output.append(filepath)
    
    return output

def github_get_file(allowed_filepath_list):

    output = []

    for allowed_filepath in allowed_filepath_list:

        url = f"https://api.github.com/repos/{GITHUB_OWNER_NAME}/{GITHUB_REPO_NAME}/contents/{allowed_filepath}"

        response = requests.get(url, headers=GITHUB_HEADERS)
        response.raise_for_status()

        data = response.json()

        content = base64.b64decode(data["content"]).decode("utf-8")

        output.append({
            "filepath": allowed_filepath,
            "content": content
        })

    return output

def call_llm(content_file_list, prompt_func):

    if isinstance(content_file_list, list):
        output = []
        for i, content_file in enumerate(content_file_list):
            print(f" -> Analyse du fichier {i+1}/{len(content_file_list)} : {content_file['filepath']}")
            
            prompt = prompt_func(content_file['filepath'], content_file['content'])
            response = GOOGLE_CLIENT.models.generate_content(
                model=GOOGLE_MODEL,
                contents=prompt,
            )
            output.append({
                "filepath": content_file["filepath"],
                "summary": response.text
            })
            
            if i < len(content_file_list) - 1:
                time.sleep(4.5) # because max rpm is 15, 4.5 to be sure not to overflow the model
                
        return output
    
    if isinstance(content_file_list, str):
        content = content_file_list
        prompt = prompt_func(content)
        response = GOOGLE_CLIENT.models.generate_content(
            model=GOOGLE_MODEL,
            contents=prompt,
        )
        return response.text

def aggregate(summary_files):

    output = ""

    for summary_file in summary_files:
        output += f"\n{summary_file['filepath']}:{summary_file['summary']}"

    return output

def save_markdown_in_file(content, output_filename):
    try:
        with open(output_filename, "w", encoding="utf-8") as f:
            f.write(content)
        print(f"✅ Fichier sauvegardé avec succès dans : {output_filename}")
    except:
        print(f"❌ Problème lors de la sauvegarde du fichier : {output_filename}")
main()