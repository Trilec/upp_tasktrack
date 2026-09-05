BASE: 6dde80187ccdb8979fa3a34d48ace213b7a37843
TASK: Finish 0.3.2 Secure MCP Tunnel usability after successful browser proof.
BUILD: 0.3.2-rc1
PROOF: Browser ChatGPT returned version 0.3.2-rc1 and tunnel_probe #2 through the live local stdio runtime.
STATUS: Tunnel transport accepted; bounded native UX/activity polish published for Windows validation.
UX: Editable/remembered tunnel ID, green Ready indicator, orange remote received/sent counter, selectable status and Copy status.
ACTIVITY: Only MCP processes launched under TASKTRACK_TUNNEL_REMOTE=1 increment diagnostic counters; ordinary local/Codex MCP traffic is excluded.
BOUNDARY: No task/dashboard schema, persistence, human lifecycle or tunnel wire-protocol change.
NEXT: Gary compiles/tests current HEAD; Curt reconnects and confirms indicators/activity; then create a real dashboard locally and read it from browser ChatGPT.
