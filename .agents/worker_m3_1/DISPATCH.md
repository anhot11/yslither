## 2026-09-22T16:48:03Z

You are the Network Stability Worker for Milestone 3 (Requirement R3) of the yslither project.
Your Working Directory: /root/.gemini/antigravity-cli/scratch/yslither/.agents/worker_m3_1
Project Directory: /root/.gemini/antigravity-cli/scratch/yslither
Original Request: /root/.gemini/antigravity-cli/scratch/yslither/.agents/ORIGINAL_REQUEST.md
Project Specification: /root/.gemini/antigravity-cli/scratch/yslither/PROJECT.md
Project Rules: /root/.gemini/antigravity-cli/scratch/yslither/.agents/rules/android-native-vulkan.md
Explorer Survey 2 Report: /root/.gemini/antigravity-cli/scratch/yslither/.agents/explorer_survey_2/handoff.md

MANDATORY INTEGRITY WARNING:
DO NOT CHEAT. All implementations must be genuine. DO NOT hardcode test results, create dummy/facade implementations, or circumvent the intended task. A teamwork_preview_auditor will independently verify your work. Integrity violations WILL be detected and your work WILL be rejected.

Mission:
Implement all fixes for Milestone 3 (Requirement R3 - Server Connection and Match Entry Stability).
Files Owned Exclusively:
- app/src/network/server.c
- app/src/network/server.h
- app/src/network/callback.c
- app/src/network/server_list.c
- app/src/network/server_list.h
- app/src/game/loop.c

Implementation Details (per Explorer Survey 2 recommendations):
1. Resilient Timeout & Failover in app/src/game/loop.c:37-70:
   When cur_t > 3.5s, force gdata->closed = true even if gdata->connection is NULL so failover logic always triggers; on retry exhaustion (connect_retry_count >= 3), cleanly disconnect and return to TITLE_SCREEN without hanging.
2. Clean Connection Lifecycle in app/src/network/server.c and loop.c:
   Always call server_disconnect(env) before initiating server_connect(env) to eliminate leaked Mongoose structures and stale sockets.
3. Thread-Safe Server List Access in app/src/network/server_list.c and server_list.h:
   Implement bool server_list_get_copy(int index, server_entry* out_entry) guarded by s_mutex to eliminate the data race with qsort() in ping_worker_thread. Update callers (e.g. title_screen.c or server selection) safely.
4. Sub-packet Buffer Bounds Protection in app/src/network/callback.c:1340-1358:
   Add bounds validation: if (m + 1 >= l) break; and if (m + len > l) break; to eliminate memory over-reads.
5. Player Snake Identity Protection in app/src/network/callback.c:363:
   Ensure incoming snake packet 's' is validated against usrs->nickname (and is not an allied feeder bot [FEED]) before assigning gdata->data.snake_id, preventing foreign snake hijacking.
6. Hotkey NULL Guards in app/src/game/loop.c:85-95:
   Add if (gdata->connection) guards before setting is_closing.

Verification:
Run ./gradlew assembleDebug to verify compilation passes with zero errors.
Write your completion report to /root/.gemini/antigravity-cli/scratch/yslither/.agents/worker_m3_1/handoff.md and notify parent (conversation ID: b673f1a9-0adf-43e6-bf42-cddae60d4e2d).
