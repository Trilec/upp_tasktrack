# Project status handoff

Use this optional document when a chatbot or agent cannot call TaskTrack directly. It describes project truth, not MCP controls, GUI geometry or dashboard JSON. Keep it bounded and update it rather than turning it into a history log.

Recommended fields:

- Project and source commit/date;
- Current phase and what is happening now;
- Milestones and measurable objectives/progress;
- Immediate next actions;
- Attention, blockers and risks;
- Verification/test evidence;
- Unresolved human decisions;
- Important recent changes.

A TaskTrack-capable agent should read this handoff, verify fresher repository evidence, reconcile conflicts, read the current dashboard if present, and update it through the normal `get_dashboard` → merge → `upsert_dashboard` flow using the exact current revision.
