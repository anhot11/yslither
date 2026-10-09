# Project: yslither Vulkan Android Game Client Overhaul

## Architecture
The yslither client is a high-performance native Android application written in C/C++ running on top of Vulkan and Dear ImGui.
- **Rendering**: Vulkan 2D pipeline rendering background grid, food orbs, snake bodies, accessories, boundary ring, and circular minimap texture quad (`app/src/rendering/`).
- **UI & HUD**: Dear ImGui overlay rendering top status badges (Tam/Size, Ping, World Coordinates, Bots count/dist) and radar minimap overlays with feeder homing lines (`app/src/game/ui_overlay.c`, `custom_controls.c`).
- **Networking**: Mongoose v7.20 WebSocket client managing single-threaded polling on the render loop, handling binary protocol packets for player snake (`app/src/network/`) and feeder bot swarm (`app/src/game/feeder.c`).
- **Bot Intelligence**: Autonomous player snake AI utilizing smooth goal navigation, food prioritization, predictive 360-degree obstacle evasion, and defensive coiling (`app/src/game/sbot.c`, `sbot.h`).
- **Feeder Swarm**: Autonomous feeder bots that establish WebSockets, solve challenge packets, home into the middle of the player snake, accelerate on approach, and explode into food mass in a continuous respawn loop (`app/src/game/feeder.c`).

## Feature Inventory
Every feature identified across user requirements (R1–R4) and survey reports:
| # | Feature | Description | Milestone | Source |
|---|---------|-------------|-----------|--------|
| 1 | F1.1 Jitter-Free Bot Steering | Smooth heading angle calculation without 32-ray snapping and low-pass filtering on `sang` | M1 | ORIGINAL_REQUEST R1, Survey 1 |
| 2 | F1.2 180° Oscillation Elimination | Continuous angular turn sign calculation avoiding ±PI flip jitter on rear/flank targets | M1 | ORIGINAL_REQUEST R1, Survey 1 |
| 3 | F1.3 Food & Corpse Prioritization | Proactive pathing and multi-factor valuation for high-value food orbs, corpse trails, and drops | M1 | ORIGINAL_REQUEST R1, Survey 1 |
| 4 | F1.4 360° Predictive Obstacle Avoidance | Dynamic hazard field keeping ≥500u buffer from enemy heads, repelling from bodies and arena rims | M1 | ORIGINAL_REQUEST R1, Survey 1 |
| 5 | F1.5 Feeder Bot Friendliness | Friendly classification for allied feeder bots so player snake does not panic or steer away | M1 | ORIGINAL_REQUEST R1, Survey 1 |
| 6 | F1.6 Defensive Coiling Overhaul | Coiling/circling maneuver when surrounded that protects small snakes and retains collision awareness | M1 | ORIGINAL_REQUEST R1, Survey 1 |
| 7 | F2.1 Feeder Connection Stagger | Staggered connection timing avoiding simultaneous connect race on spawn, eliminating code 1006 | M2 | ORIGINAL_REQUEST R2, Survey 1 |
| 8 | F2.2 Feeder Challenge Resolution | Robust binary packet '6' challenge decoding and spawn packet 's' transmission | M2 | ORIGINAL_REQUEST R2, Survey 1 |
| 9 | F2.3 Stalled Slot Watchdog | Connection & handshake watchdog to clean up and recycle stalled feeder sockets | M2 | ORIGINAL_REQUEST R2, Survey 1 |
| 10 | F2.4 Exact Feeder Identity Matching | Strict nickname matching `[FEED] #%d` preventing foreign feeder bot identity hijacking | M2 | ORIGINAL_REQUEST R2, Survey 1 |
| 11 | F2.5 Middle Body Segment Homing | Homing directly into player snake middle body coordinates `me->pts[pts_len / 2]` | M2 | ORIGINAL_REQUEST R2, Survey 1 |
| 12 | F2.6 Proximity Turbo Homing | Accelerating turbo boost when approaching player body (<2200u), conserving mass at long range | M2 | ORIGINAL_REQUEST R2, Survey 1 |
| 13 | F2.7 Continuous Respawn Loop | Clean socket teardown on death ('v'/'s') and auto-respawn after cooldown | M2 | ORIGINAL_REQUEST R2, Survey 1 |
| 14 | F3.1 Resilient Match Entry | Guaranteed failover after 3.5s timeout even when connection is NULL, preventing loading hang | M3 | ORIGINAL_REQUEST R3, Survey 2 |
| 15 | F3.2 Clean Connection Lifecycle | Proper `server_disconnect` before reconnecting, preventing Mongoose socket leakage | M3 | ORIGINAL_REQUEST R3, Survey 2 |
| 16 | F3.3 Thread-Safe Server Selection | Mutex-guarded `server_list_get_copy` eliminating data race with `ping_worker_thread` `qsort` | M3 | ORIGINAL_REQUEST R3, Survey 2 |
| 17 | F3.4 Sub-Packet Bounds Protection | Bounds checking on binary sub-packet parsing in `callback.c:1340-1358` preventing memory over-read | M3 | ORIGINAL_REQUEST R3, Survey 2 |
| 18 | F3.5 Player Identity Protection | Verify nickname on snake spawn packet 's' preventing foreign snake camera/death hijacking | M3 | ORIGINAL_REQUEST R3, Survey 2 |
| 19 | F3.6 Hotkey NULL Guard | Safe NULL checks on `gdata->connection` during hotkey/touch quit and restart | M3 | ORIGINAL_REQUEST R3, Survey 2 |
| 20 | F4.1 1:1 Minimap Coordinate Mapping | World-to-minimap transform using dynamic `flux_grd` boundary, matching arena rim without drift | M4 | ORIGINAL_REQUEST R4, Survey 3 |
| 21 | F4.2 Unified Player Marker | Single distinct player marker on minimap without double-dot divergence | M4 | ORIGINAL_REQUEST R4, Survey 3 |
| 22 | F4.3 Vibrant Neon Green Feeders | Neon green feeder dots (`IM_COL32(0, 255, 60, 255)`) with animated directional homing lines | M4 | ORIGINAL_REQUEST R4, Survey 3 |
| 23 | F4.4 Clean Top HUD Badges | Badges for Tam, Ping, Coords, Bots without legacy text bleed underneath | M4 | ORIGINAL_REQUEST R4, Survey 3 |
| 24 | F4.5 Minimap Touch Un-Occlusion | Compact minimap sizing and touch button layout ensuring radar visibility | M4 | ORIGINAL_REQUEST R4, Survey 3 |
| 25 | F5.1 E2E Test Infrastructure | Test suite and harness covering Tiers 1-4 for bot, feeders, network, minimap | M5 | ORIGINAL_REQUEST AC, Survey 1-3 |
| 26 | F5.2 Physical Device Verification | Clean build, install on `192.168.7.12:33203`, logcat traces, screenshots proving all criteria | M5 | ORIGINAL_REQUEST AC, Survey 3 |

## Milestones
| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| M1 | Autonomous Player Bot AI Overhaul (R1) | F1.1, F1.2, F1.3, F1.4, F1.5, F1.6 | none | IN_PROGRESS |
| M2 | Feeder Bots Match Entry & Feeding Cycle (R2) | F2.1, F2.2, F2.3, F2.4, F2.5, F2.6, F2.7 | M1 | PLANNED |
| M3 | Server Connection & Match Entry Stability (R3) | F3.1, F3.2, F3.3, F3.4, F3.5, F3.6 | none | IN_PROGRESS |
| M4 | Minimap & HUD Rendering Precision (R4) | F4.1, F4.2, F4.3, F4.4, F4.5 | none | IN_PROGRESS |
| M5 | E2E Testing Suite & Physical Device Verification | F5.1, F5.2 (Tiers 1-4, APK build, install, live verification on 192.168.7.12:33203) | M1, M2, M3, M4 | IN_PROGRESS (Test Writer) |

## Interface Contracts
### Network (`server.c`, `server.h`) ↔ Game Loop (`loop.c`)
- `void server_connect(tenv* env)`: Connects to current server `usrs->ipv4`, initializing Mongoose connection cleanly.
- `void server_disconnect(tenv* env)`: Closes active connection, drains pending events, and sets `gdata->connection = NULL`.
- `void server_poll(tenv* env)`: Polls Mongoose event manager for main client connection.
- `bool server_list_get_copy(int index, server_entry* out_entry)`: Thread-safe server retrieval copying data under `s_mutex`.

### Feeder Swarm (`feeder.c`, `feeder.h`) ↔ Player & Renderer
- `void feeder_init(void)`: Initializes feeder state and sets `s_last_bot_spawn = glfwGetTime()`.
- `void feeder_update(tenv* env)`: Performs rate-limited spawning, handshake timeout checks, middle-segment tracking (`me->pts[pts_len / 2]`), turbo boost, and death respawn.
- `int feeder_get_active_count(void)`: Returns number of currently alive and connected feeder bots.
- `float feeder_get_closest_distance(tenv* env)`: Returns distance to closest feeder bot from player snake.
- `const feeder_bot* feeder_get_bots(void)`: Returns read-only pointer to feeder swarm pool for minimap rendering.

### Player Bot AI (`sbot.c`, `sbot.h`) ↔ Simulation (`oef.c`, `input.c`)
- `void sbot_init(void)`: Resets bot navigation state.
- `void sbot_go(tenv* env)`: Computes smooth goal vector `B.goal`, applies 360-degree obstacle hazard field (skipping allied feeders), maintains ≥500u buffer from enemy heads, and writes smoothed output `bot->output.xm`, `bot->output.ym`, `bot->output.accel`.
- Allied feeder check: `bool is_allied_feeder(const snake* s)` returning true if `strncmp(s->nk, "[FEED]", 6) == 0`.

### Minimap & HUD (`ui_overlay.c`, `custom_controls.c`)
- Minimap world-to-screen transform:
  - `cx_world = (gdata->data.grd > 0.0f) ? gdata->data.grd : 21600.0f;`
  - `rad_world = (gdata->data.flux_grd > 0.0f) ? gdata->data.flux_grd : (cx_world * 0.98f);`
  - `nx = (world_x - cx_world) / rad_world; ny = (world_y - cx_world) / rad_world;`
  - `screen_x = mm_cx + nx * mm_rad; screen_y = mm_cy + ny * mm_rad;`
- Feeder bot rendering: Neon green circle `IM_COL32(0, 255, 60, 255)` + animated directional homing guide line towards player snake.
- Top HUD: 4 modern badges rendered cleanly without legacy desktop text overlap.

## Code Layout
- `app/src/game/sbot.c`, `app/src/game/sbot.h`: Autonomous snake bot AI logic. Owned by M1.
- `app/src/game/input.c`: Player input and angle packet transmission. Owned by M1.
- `app/src/game/feeder.c`, `app/src/game/feeder.h`: Feeder bots manager. Owned by M2.
- `app/src/network/server.c`, `server.h`, `callback.c`, `server_list.c`, `server_list.h`: Network stack. Owned by M3.
- `app/src/game/loop.c`: Game main loop and connection state machine. Owned by M3.
- `app/src/game/ui_overlay.c`, `custom_controls.c`: Dear ImGui HUD, minimap, controls. Owned by M4.
- `test/`, `scripts/`: Verification scripts, test runners, logcat parsers. Owned by M5.
