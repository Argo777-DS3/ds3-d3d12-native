---
name: resume-thread
description: Start or resume a thread on the DS3 project (a fresh thread, a new workstream, after a context compression or a PC restart). Use before any technical decision, build or live run in a new conversation.
---

# Resume a thread

1. Read, in order: `.project/INDEX.md`, `.project/STATE.md` (its resume rows), the current workstream in
   `.project/ROADMAP.md`, `.project/NATIVE-D3D12-RESET-MANDATE.md`, `decisions/IN-FORCE.md` and `.project/WORKFLOW.md`. After a
   context compression all six are reread before any technical decision, implementation or live test; a conversation summary never replaces them.
2. Check where the work stands: `git branch --show-current`, `git worktree list`, `git log --oneline -5 main` and the branch
   named in STATE's « Pilot branch » row. Never assume `main` is newest; reconcile before repeating any acquisition.
3. Check the other sessions on the PC before a build or a live run (another thread may be testing): the thread list or
   `list_sessions`, and whether `DarkSoulsIII` or `java` (headless Ghidra) runs.
4. In a new checkout or worktree: `pwsh -NoProfile -File scripts/Connect-EvidenceStore.ps1`; before reading or changing
   `native/`, `pwsh -NoProfile -File scripts/New-CompileCommands.ps1` (clangd, WORKFLOW).
5. Opening a workstream, or continuing one in this fresh thread: create or name its investigation
   (`scripts/New-Investigation.ps1` for a new one), then in the first commit write this thread's branch (the bare name) in STATE's
   « Pilot branch » row and the workstream in « Active workstream ». Copy the study that guides
   it (the study thread's report) into the investigation in that commit.
6. Not the pilot (a study, an audit, a review): leave « Pilot branch » alone.
