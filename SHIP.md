# 🚀 SHIP Protocol

When this file is referenced, Gemini CLI must perform the following deployment workflow:

### 1. Pre-Flight Check
- Run `git status` to identify any uncommitted changes.
- Run `git log origin/main..HEAD` to review all local commits that will be pushed.
- Confirm all remote builds associated with these commits have passed.

### 2. Final Staging (If needed)
- Ensure all final tweaks are committed locally.

### 3. Pushing
- Execute `git push origin main` to synchronize the local history with the remote.
- Confirm successful completion with a final `git status`.

---
**Status**: Ready to ship when you are.
