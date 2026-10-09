# BRIEFING — 2026-09-22T16:44:45Z

## Mission
Investigate the codebase for Requirement R3 (Server Connection and Match Entry Stability) and network architecture.

## 🔒 My Identity
- Archetype: explorer
- Roles: Network Protocol Explorer
- Working directory: /root/.gemini/antigravity-cli/scratch/yslither/.agents/explorer_survey_2
- Original parent: b673f1a9-0adf-43e6-bf42-cddae60d4e2d
- Milestone: Explorer Survey

## 🔒 Key Constraints
- Read-only investigation — do NOT implement
- Strictly read-only on project source code files; only write reports/analysis in working directory
- Produce 5-component handoff report (Observation, Logic Chain, Caveats, Conclusion, Verification Method) in handoff.md
- Send message to parent (b673f1a9-0adf-43e6-bf42-cddae60d4e2d) when ready

## Current Parent
- Conversation ID: b673f1a9-0adf-43e6-bf42-cddae60d4e2d
- Updated: not yet

## Investigation State
- **Explored paths**:
  - `app/src/network/server.h`, `server.c`
  - `app/src/network/server_list.h`, `server_list.c`
  - `app/src/network/callback.h`, `callback.c`
  - `app/src/external/mongoose.h`, `mongoose.c`
  - `app/src/game/loop.h`, `loop.c`
  - `app/src/game/game_data.h`, `game_data.c`
  - `app/src/game/feeder.h`, `feeder.c`
  - `app/src/game/input.h`, `input.c`
  - `app/src/game/oef.h`, `oef.c`
  - `app/src/game/redraw.h`, `redraw.c`
  - `app/src/game/custom_controls.h`, `custom_controls.c`
  - `app/src/game/ui_overlay.h`, `ui_overlay.c`
  - `app/src/ui/title_screen.h`, `title_screen.c`
  - `app/src/constants.h`, `user.h`, `main.c`, `android_main.c`, `CMakeLists.txt`
  - Verification logs: `logcat_v25_verified.log`, `verify_v25.py`
- **Key findings**:
  1. WebSocket stack uses Mongoose 7.20 non-blocking client polled synchronously on the render thread (`mg_mgr_poll(&gdata->network_manager, 0)`).
  2. Data race condition in `server_list.c`: `server_list_get(index)` accesses `s_servers` without holding `s_mutex`, while detached background thread `ping_worker_thread` executes `qsort(s_servers, ...)`.
  3. Hang in `case CONNECTING` (`loop.c:37-41`): If `gdata->connection` is NULL or doesn't trigger `gdata->closed`, 3.5s timeout logs error every frame but never triggers failover, freezing loading bar forever.
  4. Dirty connection leak: `server_connect()` is repeatedly called during failover (`loop.c:64`) and JUGAR (`title_screen.c:542`) without calling `server_disconnect()`, leaving previous connections lingering in `gdata->network_manager`.
  5. Missing boundary checks in `callback.c:1344-1353` for bundled sub-packets (`m + 1 >= l` and `m + len > l`), causing buffer over-read on fragmented packets.
  6. Player snake identity hijacking: `callback.c:363` assumes the first snake packet `'s'` received while `conn == CONNECTING` is the player snake without checking nickname match (`o.nk == usrs->nickname`).
  7. NULL pointer dereferences in `loop.c:88, 92` on `gdata->connection->is_closing = true` when hotkeys are pressed and connection is closed.
- **Unexplored areas**: None within the network protocol and match entry scope.

## Key Decisions Made
- Completed systematic trace of questions 1-9.
- Preparing detailed 5-component `handoff.md` with complete code quotes, line numbers, call graphs, and proposed interfaces.

## Artifact Index
- DISPATCH.md — Recorded dispatch instructions
- BRIEFING.md — Situational awareness and working memory
- progress.md — Liveness heartbeat and step tracking
- handoff.md — Final comprehensive report for orchestrator/workers
