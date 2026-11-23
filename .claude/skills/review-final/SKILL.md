---
name: review-final
description: "Brief and launch the reviews before a merge into main (WORKFLOW « Roles and delegation » and « Merging into `main` », D-0198, D-0268, D-0283): the final-reviewer subagent (Opus 5.5 xhigh) on the final tree, briefed to find what is wrong from the diff, the goal and the criteria, then the integrity-reviewer. Use in skill merge-workstream's step 4."
---

# The reviews before a merge

The rules are WORKFLOW « Reviewers » (D-0198; D-0283, Thomas 2026-11-15) and « Guarding the judge » (D-0268). In order:

1. **The final tree**: the optimisation pass done and committed, `main` merged into the branch (`git merge main`), the tree
   built with its unit tests on that commit (a review of an unbuilt tree is not a merge gate). Note `main`'s hash and the
   branch's head.
2. **The final review's brief**, exactly:
   - the repository path and the range `main...<head>`, read-only;
   - **the goal**: the CONTRACT's question and scope (its path), the tranche's target in the plan's words, the decisions of
     Thomas it applies by number; a target, not a report;
   - **the exit criteria**, each checkable in the code or the evidence;
   - where to look: the code files, the decompiled references, the evidence folders of the runs (as places, not as results);
   - the order: « Find what is wrong: defects, criteria not met, claims of the records (RESULT, STATE, ROADMAP, commit messages)
     that the code or the evidence does not support, stale or contradictory documentation. Read the code before the records. Report
     High / Medium / Low with file:line and a failure scenario; then what held. »
   Leave out (D-0283): the pilot's account of what it did or fixed, the step reviews' findings and answers, the runs' outcome in
   the pilot's words, any « verified » or « works ».
3. **Launch** the `final-reviewer` (xhigh) in the background. After it (WORKFLOW « Guarding the judge », D-0268: after the
   final review), the `integrity-reviewer` on the same range with the RESULT section of the branch: its object is the records'
   claims, so it gets them, as claims to check against the evidence.
4. **Answer** every finding: fixed (a commit; a code fix gets its own step review, skill `review-step`, and a new build), or left
   with its reason; both reviews and their outcome recorded in the RESULT's « Reviews at the merge », the severities as the
   reviewers gave them. Then skill merge-workstream's step 6 on.
