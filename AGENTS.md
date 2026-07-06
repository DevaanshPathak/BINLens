# AGENTS.md

Guidance for automated agents working in this repository.

## Repository Rules

- Do not open pull requests unless the user explicitly asks for one.
- Do not create feature branches unless the user explicitly asks for another branch.
- Default workflow is direct commits to `main`, but commit only when the user explicitly asks for a commit.
- Push to `main` only when the user explicitly asks for a push.
- If the user requests a pull request or another branch, only change branches as requested; do not open a PR unless that is separately and explicitly requested.
- Do not write implementation code when the user asks for planning or documentation artifacts only.
- Write devlogs only when the user explicitly asks for a devlog.
- Store devlogs in `devlogs/` as numbered Markdown files using the next integer filename, such as `1.md`, `2.md`, `3.md`.
- Keep each devlog under 4,000 characters.
- Keep the project C11, POSIX-compatible, Makefile-based, and dependency-free unless the user changes those constraints.
