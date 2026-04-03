# 🚀 SHIP Protocol

When this file is referenced, Gemini CLI must perform the following deployment workflow:

### 1. Pre-Flight Check
- Run `git status` to identify all modified and untracked files.
- Run `git diff HEAD` to review all pending changes.
- Review `git log -n 5` to maintain consistency with previous commit styles (e.g., Conventional Commits).

### 2. Staging
- Stage all relevant files (`git add .` or specific files as appropriate).
- Exclude transient log files (e.g., `log.txt`, `log2.txt`).

### 3. Committing
- Draft a concise, high-signal commit message following the project's style.
- Summarize the "Why" and "What" of the batched changes.

### 4. Pushing
- Execute `git push` to the remote repository.
- Confirm successful completion with a final status check.

---
**Status**: Ready to ship when you are.
