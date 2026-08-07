# TT-002-W1 — Windows Acceptance Matrix

This is the authoritative Windows validation for the first V0.2 semantic-question build after TT-001-W1.

## Source boundary

Gary must pull `main`, confirm the exact TaskTrack commit supplied by the supervisor, and record current `upp_Ui`, `upp_animation`, and `upp_statemachine` SHAs before building. Do not reset newer dependencies merely to match an older reference unless a proven compatibility issue requires a separate corrective task.

Do not commit or push source repairs during the acceptance run. Return the first useful failure and diagnosis.

## Automated build matrix

From `E:\apps\github\upp_tasktrack`:

```powershell
powershell -ExecutionPolicy Bypass -File .\verify.ps1 -UppRoot E:\upp-18468
```

Required Release results:

- TaskTrack/App PASS
- TaskTrack/Mcp PASS
- tests/TaskTrackTests PASS
- examples/TaskTrackExample PASS
- TaskTrackTests runtime PASS
- TaskTrackMcp `--selftest` PASS

Then build and run the same packages with CLANG x64 Debug. Any U++ assertion, access violation, heap diagnostic or abnormal shutdown is a FAIL.

Close a running `build\TaskTrack.exe` before rebuilding it; Windows executable locking is not a source failure.

## Demo integrity

Run:

```powershell
.\build\TaskTrackExample.exe
```

Open the printed task path with `TaskTrack.exe --task`.

The generated document must be schema V2 and contain exactly **18** questions, one each of:

1. confirm
2. single_choice
3. multi_choice
4. select
5. list_select
6. text
7. notes
8. number
9. amount
10. range
11. rating
12. color
13. gradient
14. position
15. direction
16. rank_order
17. hierarchy_select
18. curve

## Shell / compactness

Mechanically confirm:

- no redundant “Verification” section heading above the card grid;
- question title and subtitle are in each restrained group/card header;
- no generic Note row appears beneath every question;
- categories appear only because the demo has several categories;
- category buttons wrap cleanly;
- wide window produces approximately three question columns where card widths permit;
- medium width falls to roughly two;
- narrow width falls to one;
- no overlap, clipping, inaccessible control, or broken vertical remeasurement occurs while wrapping;
- footer remains compact and usable.

Curt owns aesthetic judgement of exact proportions, font hierarchy, card weight and spacing.

## Per-type interaction

Exercise every control at least once and then reopen the task to confirm the response persisted.

- confirm — choose Yes and No; only one active.
- single_choice — choose among the small choices; only one active; agent recommendation is visible but does not pre-answer.
- multi_choice — toggle multiple independent options; structured array persists.
- select — choose a dropdown item.
- list_select — select multiple list rows.
- text — type a short value.
- notes — type multi-line qualification text.
- number — type/spin a bounded number.
- amount — move slider and edit value; both remain synchronized.
- range — drag both thumbs; low must never exceed high; keyboard left/right on selected thumb should work.
- rating — choose one score; one active.
- color — choose preset, then custom colour; selected value persists as colour text.
- gradient — choose each visual candidate; selected frame changes.
- position — choose 3×3 placements; one active.
- direction — choose 8-way directions; center is not selectable.
- rank_order — drag rows, accept order, reopen and confirm order persists.
- hierarchy_select — select tree node and reopen.
- curve — move Bézier handles, accept curve, reopen and confirm the four structured control-point values persist.

## Structured result

Complete all required questions and submit. Retrieve through MCP/JSON and confirm representative `answer.data` types remain structured:

- confirm boolean
- multi_choice array
- number/amount/rating number
- range object with low/high
- rank_order ordered array
- hierarchy node id/array
- curve four-number array

Do not accept a build that collapses these to display-only strings.

## V0.1 compatibility

Open at least one task created by V0.1 / TT-001-W1 if still available. It must load without schema error. Old `pass_fail`/`multiline` style data must remain visible/answerable through their migrated semantic forms. Also verify a V0.1 `color` witness carrying `expected_color` still appears as a Match/Different/Unsure verdict rather than being reinterpreted as a colour chooser. Saving may rewrite the document as schema V2.

## Manual autosave acceptance

TT-001-W1 only proved this at persistence/code level. This pass must exercise it visibly:

1. enter distinctive values in at least text, multi-choice, range and position;
2. do not press Save;
3. allow debounced autosave;
4. close normally;
5. reopen same task;
6. confirm all four values remain.

Then repeat one edit with explicit Save and reopen.

## Reminder acceptance

TT-001-W1 did not manually exercise the modal reminders. This pass must:

- set reminder to 1 minute;
- leave active task idle until prompt appears;
- choose Continue and verify state remains/returns `in_progress`;
- repeat and choose Pause;
- enable paused reminders, remain paused, and confirm a reminder may appear;
- on a disposable task choose Close task and confirm explicit `closed`;
- verify no task closes merely because time elapsed.

## Agent nudge / restart durability

With Agent nudge enabled, leave an unfinished task inactive for at least one minute and poll it through MCP. The reminder may appear, but evidence and task state must not change automatically.

Create another task with `launch:false`, restart the MCP process, resolve by `task_id`, open/answer/complete it, restart MCP again, and retrieve completed structured evidence.

## Report

Return:

- exact TaskTrack SHA and branch
- dependency SHAs
- Release/Debug build results for all four packages
- full TaskTrackTests summary
- MCP selftest result
- generated demo path
- 18/18 semantic control interaction result
- responsive 3/2/1 wrapping result
- V0.1 migration result
- GUI autosave/reopen result
- explicit Save result
- reminder/paused-reminder/no-auto-close result
- agent nudge result
- MCP restart durability result
- whether GUI is ready for Curt visual review
- first exact failure + diagnosis if anything fails
- final `git status --short`
