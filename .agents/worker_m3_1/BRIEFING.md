# BRIEFING — 2026-09-22T16:48:15Z

## Mission
Implement all fixes for Milestone 3 (Requirement R3 - Server Connection and Match Entry Stability) across network and game loop modules.

## 🔒 My Identity
- Archetype: worker
- Roles: implementer, qa, specialist
- Working directory: /root/.gemini/antigravity-cli/scratch/yslither/.agents/worker_m3_1
- Original parent: b673f1a9-0adf-43e6-bf42-cddae60d4e2d
- Milestone: Milestone 3 (Requirement R3)

## 🔒 Key Constraints
- Do not cheat: genuine logic, real state and behavior, no fake outputs or facades.
- Files Owned Exclusively:
  - app/src/network/server.c
  - app/src/network/server.h
  - app/src/network/callback.c
  - app/src/network/server_list.c
  - app/src/network/server_list.h
  - app/src/game/loop.c
- Do not edit files owned by other workers without care. Note: check title_screen.c if server_list_get_copy needs callers updated or if server_list callers are in our owned files. Let's inspect who calls server_list.
- Minimal change principle.
- ./gradlew assembleDebug must compile cleanly.

## Current Parent
- Conversation ID: b673f1a9-0adf-43e6-bf42-cddae60d4e2d
- Updated: not yet

## Task Summary
- **What to build**:
  1. Resilient Timeout & Failover in app/src/game/loop.c:37-70: force closed = true when cur_t > 3.5s, clean return to TITLE_SCREEN on retry exhaustion.
  2. Clean Connection Lifecycle in app/src/network/server.c and loop.c: call server_disconnect before server_connect.
  3. Thread-Safe Server List Access in app/src/network/server_list.c and server_list.h: bool server_list_get_copy(int index, server_entry* out_entry) guarded by s_mutex. Update callers safely.
  4. Sub-packet Buffer Bounds Protection in app/src/network/callback.c:1340-1358: add bounds validation (m + 1 >= l, m + len > l).
  5. Player Snake Identity Protection in app/src/network/callback.c:363: validate packet 's' against usrs->nickname (and not [FEED]) before assigning gdata->data.snake_id.
  6. Hotkey NULL Guards in app/src/game/loop.c:85-95: add if (gdata->connection) guards before setting is_closing.
- **Success criteria**: ./gradlew assembleDebug succeeds, all 6 fixes properly implemented and verified.
- **Interface contracts**: /root/.gemini/antigravity-cli/scratch/yslither/PROJECT.md
- **Code layout**: app/src/network, app/src/game

## Key Decisions Made
- [Initial assessment]

## Artifact Index
- handoff.md — Final completion report
- progress.md — Liveness heartbeat and progress log
- DISPATCH.md — Assignment from orchestrator

## Change Tracker
- **Files modified**: None yet
- **Build status**: Untested
- **Pending issues**: None

## Quality Status
- **Build/test result**: Pending
- **Lint status**: Clean
- **Tests added/modified**: Pending

## Loaded Skills
- None
