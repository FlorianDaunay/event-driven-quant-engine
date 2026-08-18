import os

GITHUB_KEY = os.getenv("PAT_GITHUB_KEY")
GOOGLE_AI_KEY = os.getenv("GOOGLE_AI_KEY")
COMMIT_HASH = os.getenv("COMMIT_HASH", "local-dev")
COMMIT_MSG = os.getenv("COMMIT_MSG", "Dév local sans commit GitHub")