---
name: review-step
description: "Brief and launch the review of one step of a DS3 native workstream (WORKFLOW « Roles and delegation », D-0198, D-0283) with the step-reviewer subagent (Opus 5.5 high), so that it looks for what is wrong from the diff, the goal and the criteria alone. Use after every step that changed code, and for the fixes a review asked for."
---

# A step review

The rule is WORKFLOW « Roles and delegation », its « Reviewers » (D-0198; D-0283, Thomas 2026-11-15: « Trouve ce qui ne va pas »,
and the diff and the goal, not the pilot's summary). In order:

1. **Commit the step** first: the reviewer reads a commit range, never a working tree that may still move. Note the range
   (`<before>..<after>`), the step's code only when documentation commits sit in between (name the code files then).
2. **Write the brief** with exactly these parts, and nothing else:
   - the repository path (the worktree) and the diff range, read-only;
   - **the goal**: what the step must make true, in the plan's or the contract's words (the CONTRACT, the RESULT's plan, a
     decision of Thomas's by its number), stated as a target, not as something done;
   - **the exit criteria**: what must hold for the step to pass, each one checkable in the code;
   - the places in the code or the decompiled reference where a criterion can be checked (files, functions, addresses);
   - the order: « Find what is wrong: defects, criteria not met, claims in the code or its comments that the code does not
     support. Read the code before any record (RESULT, commit messages). Report High / Medium / Low with file:line and a
     failure scenario; then what held. »
3. **Leave out** (D-0283): what was done, what was fixed and how, the earlier reviews' findings and their answers, how the
   runs went, any « verified », « confirmed », « works ». When the step answers a review, give its findings as criteria to
   check (« the light override no longer reads B at t <= 0 »), not as a report of their fixes.
4. **Launch** the `step-reviewer` subagent with that brief, in the background (`run_in_background`), and go on with work that
   does not touch the files under review.
5. **Answer** every finding: fixed (a new commit, and a new step review when the fix changes code), or left with its reason in
   the RESULT under the step's « step review ». Report the outcome in the RESULT as the reviewer gave it, its severities
   unchanged.
