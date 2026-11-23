---
name: review-security
description: "Brief and launch the security review of the DS3 mod's own code with the security-reviewer subagent (Opus 5.5 high, read-only; Thomas, 2026-11-23), on a branch's diff or as a whole sweep, so that it looks for what could harm a player who installs the mod or leak if the repository were published. Use on a branch that touches the launchers, the files the module reads, the libraries it loads, the saves or the crash dumps, and for a sweep before the mod is shared."
---

# A security review

The reviewer is the subagent `security-reviewer` (`.claude/agents/security-reviewer.md`); the earlier findings are in
`.project/studies/security-audit/REPORT.md`. When it runs is Thomas's to settle (WORKFLOW's list of agents); the description
names the branches it suits. In order:

1. **Commit** what is to be reviewed: the reviewer reads a commit range or the tree at a commit, never a working tree that may
   still move. Note the range (`main...<branch>`) or the commit for a sweep.
2. **Write the brief** with exactly these parts, and nothing else:
   - the repository path (the worktree), read-only, and the mode: the diff range, or `sweep` with the commit;
   - what the change is for, in the plan's or Thomas's words (one line), so the reviewer knows which inputs are new;
   - the report to read after the code: its findings and its « Left » only (`.project/studies/security-audit/REPORT.md`), each
     open point to be said still open or changed;
   - the order: « Find what in our code could harm a player who installs the mod or leak if published. Read the code before
     any record. Report high / medium / low with file:line, the attacker or the accident, and the
     smallest fix; then what held. The original game, its servers and its anti-cheat are out of scope. »
3. **Leave out** (as D-0283 for the other reviewers): what was done or fixed and how, « verified », « safe », « works », the
   earlier reviews' answers. When the change answers a security finding, give the finding as a criterion to check (« the witness
   files are not read when the controller's bound is 0 »), not as a report of its fix.
4. **Launch** `security-reviewer` with that brief in the background (`run_in_background`), and go on with work that does not
   touch the files under review.
5. **Answer** every finding: fixed (a new commit, a step review when the fix changes code, and the reviewer again on the fix),
   or left with its reason. A sweep's findings, and every finding left open, go into the report's « Left » section with their
   severities as the reviewer gave them; a fix that needs a live check (a library still loads, a launcher still starts) waits
   for the start check of the merge (skill `merge-workstream`).
