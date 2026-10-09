# Handoff Report: Bot AI & Feeder Swarm Technical Survey (Requirements R1 & R2)

**Explorer**: Bot AI and Feeder Survey Agent  
**Date**: 2026-09-22  
**Target System**: yslither Vulkan Native Android Game Client  
**Scope**: Requirement R1 (Autonomous Player Bot AI Overhaul) and Requirement R2 (Feeder Bots Match Entry and Feeding Cycle)

---

## 1. Observation

### 1.1 Architecture & Entry Points
* Direct inspection of the project structure shows native C client code under `app/src/` with Vulkan rendering (`app/src/rendering/`), Dear ImGui UI overlay (`app/src/game/ui_overlay.c`, `app/src/cimgui/`), Mongoose networking (`app/src/external/mongoose.c`, `app/src/network/`), and game simulation loops (`app/src/game/loop.c`, `app/src/game/oef.c`).
* In `app/src/game/loop.c` lines 73–83 (`case CONNECTED:`), frame execution order is:
  1. `time_step(env);` (line 75)
  2. `flight_recorder_record_frame(env);` (line 76)
  3. `input(env);` (line 77) — reads `gdata->bot.output.xm` / `ym` and sends angle packet to server.
  4. `server_poll(env);` (line 78) — polls player Mongoose network manager `gdata->network_manager`.
  5. `feeder_update(env);` (line 79) — updates feeder bot connections and homing navigation.
  6. `oef(env);` (line 80) — updates entity physics and calls `sbot_go(env);` (lines 436–440 in `oef.c`).
  7. `redraw(env);` (line 81) — Vulkan rendering.
  8. `ui_overlay(env);` (line 82) — ImGui HUD, minimap overlays, touch controls.

### 1.2 Player Bot AI (R1) Observations
* **Location & State**: `app/src/game/sbot.h` defines `typedef struct sbot { struct { float xm; float ym; bool accel; } output; } sbot;`. Core AI logic resides in `app/src/game/sbot.c` (1,582 lines), maintaining static state `static bot_state B;` (lines 79–120).
* **Steering Output & Quantization**:
  * In `app/src/game/sbot.c` lines 1427–1430:
    ```c
    float dx = B.goal.x - B.x;
    float dy = B.goal.y - B.y;
    bot->output.xm = (int)roundf(dx * 10.0f);
    bot->output.ym = (int)roundf(dy * 10.0f);
    ```
  * In `app/src/game/input.c` lines 97–119, `me->eang = atan2f(ym, xm);`. When `(want_e && gdata->data.ctm - gdata->data.last_e_mtm > 50) || heartbeat_e`:
    ```c
    ang = atan2f(ym, xm);
    ang = fmodf(ang, PI2);
    if (ang < 0) ang += PI2;
    int sang = (int)floorf(251.0f * ang / PI2);
    if (sang < 0) sang = 0;
    if (sang > 250) sang = 250;
    if (sang != gdata->data.lsang || heartbeat_e) {
      gdata->data.lsang = sang;
      uint8_t pkt = (uint8_t)sang;
      mg_ws_send(connection, &pkt, 1, WEBSOCKET_OP_BINARY);
    }
    ```
* **Raycast Snapping & Oscillation Mechanism**:
  * `app/src/game/sbot.c` uses `MAXARC 32` sectors (`ARC_SIZE = PI2 / 32.0f = ~11.25°` per sector).
  * In `sbot_go` lines 1357–1362:
    ```c
    bool in_collision = check_collision(gdata);
    if (!in_collision) {
      delay_action(gdata);
      evaluate_best_evasion_heading(gdata);
    }
    ```
  * In `delay_action` (lines 1147–1233), `compute_food_goal(gdata)` selects a food target and sets `B.goal`. BUT immediately at line 1361, `evaluate_best_evasion_heading(gdata)` executes and overwrites `B.goal = heading_abs(best_angle);` (line 609), where `best_angle` is snapped to one of 32 discrete ray angles (`k * ARC_SIZE`).
  * In `delay_action` lines 1171–1178 (rear/flank food target pathing):
    ```c
    float d_turn = ang_between(f_ang, B.ang);
    float turn_sign = (d_turn >= 0.0f) ? 1.0f : -1.0f;
    float lead_ang = B.ang + turn_sign * fminf(fabsf(d_turn), ((float)M_PI * 0.55f));
    B.goal = (v2){roundf(B.x + cosf(lead_ang) * 320.0f),
                  roundf(B.y + sinf(lead_ang) * 320.0f)};
    ```
    When a target is nearly behind (`d_turn ≈ ±PI`), floating-point noise causes `d_turn` to alternate between `+3.1415` and `-3.1415` on consecutive frames. This flips `turn_sign` between `+1.0` and `-1.0`, instantaneously flipping `lead_ang` by `1.10 * PI` (198 degrees), causing severe 180° head oscillation / jitter.
* **Collision Detection & Buffers**:
  * In `get_collision_points` (lines 395–466), obstacles are categorized as:
    * `type == 0`: Enemy heads (`hx, hy` projected with buffer `head_buffer = B.head_circle.r * 1.6f`, plus forward projection for `t = 2.5` to `28.0` frames).
    * `type == 1`: Enemy body segments (sampled up to 1400px, radius `sr * 1.45f`).
    * `type == 2`: Arena rim wall (`near_wall` within 1400px of `flux_grd`, radius `BORDER_PT_RADIUS * 1.5f`).
  * In `evaluate_best_evasion_heading` lines 509–515:
    ```c
    if (cp->type == 0 && (vx * vx + vy * vy) < (800.0f * 800.0f)) {
      float head_ang = atan2f(vy, vx);
      if (fabsf(ang_between(ray_ang, head_ang)) < ((float)M_PI * 0.50f)) {
        hazard_factor *= 0.05f;
      }
    }
    ```
  * Note: `get_collision_points` iterates all snakes (`gdata->data.snakes`) where `s->id != B.id`. Feeder bots (`s->nk` matching `"[FEED]"`) are NOT excluded; they are treated as hostile enemy snakes.
* **Defensive Coiling**:
  * `check_encircle` (lines 677–706) triggers when a single snake occupies ≥ 35% of sectors (11/32) or surrounding obstacles occupy ≥ 42% of sectors (14/32) within `ed = fmaxf(750.0f, B.radius * 34.0f)`.
  * For small snakes (e.g. initial spawn `Tam: 10`, `me->pts` length ≤ 6), `to_circle` (lines 1058–1087) loop `for (int i = 0; i < pn - 6; i++)` is never entered. It falls through to line 1086:
    ```c
    B.goal = heading_rel(B.circle_dir * ((float)M_PI * 0.55f));
    ```
  * While in `stage == 1` (`to_circle`) and `stage == 2` (`follow_circle_self`), lines 1350–1356 completely skip `check_collision(gdata)`. Small snakes trapped in `stage == 1` spin blindly in place without collision evasion.

### 1.3 Feeder Bots (R2) Observations
* **Location & State**: Defined in `app/src/game/feeder.h` and `app/src/game/feeder.c`. Feeder swarm pool: `static feeder_bot s_bots[MAX_FEEDER_BOTS];` (`MAX_FEEDER_BOTS = 4`). Mongoose manager: `static struct mg_mgr s_feeder_mgr;`.
* **Connection Lifecycle**:
  * Feeder bots spawn in `feeder_update` lines 319–344 if `!bot->c && !bot->connecting && now >= bot->cooldown_until`.
  * Connection stagger check: `if (now - s_last_bot_spawn < 3.0) continue;`.
  * In `feeder_init` (line 230) and `feeder_update` on game disconnect (line 253), `s_last_bot_spawn` is set to `0.0`. When player connects (`gdata->conn == CONNECTED`), `now - s_last_bot_spawn` evaluates to `now - 0.0 > 3.0`, causing Bot 0 to connect on the very first frame of player spawn without delay relative to the player socket.
  * In `feeder_ws_cb` (line 172): on `MG_EV_WS_OPEN`, `bot->connecting = false;`.
  * If a connection stalls after `MG_EV_WS_OPEN` (e.g., waiting for challenge '6' or spawn response 's'), `bot->c != NULL`, `bot->connecting == false`, and `bot->alive == false`. Because `bot->c != NULL`, line 319 never respawns it; because `bot->alive == false`, lines 347–392 never ping or steer it. There is no timeout watchdog, permanently stalling slot `i`.
* **Challenge Solving & Spawn Packet**:
  * Packet `'6'` (lines 54–76): Decoded via `decode_secret(pkt, len, secret)` (`callback.c:30–85`). 27-byte response sent via `mg_ws_send(bot->c, secret, 27, WEBSOCKET_OP_BINARY)`.
  * Immediately sends spawn packet `'s'`: 64-byte buffer with `CLIENT_VERSION = 291`, `cwa` signature (20 bytes), skin `8` (lime green), nickname `[FEED] #%d`, accessory `0`, custom skin flag `0`.
* **Snake ID Adoption Defect**:
  * In `feeder_bot_on_packet` lines 86–90:
    ```c
    if (strncmp((const char*)(pkt + 23), bot->name, strlen(bot->name)) == 0 ||
        strncmp((const char*)(pkt + 23), "[FEED]", 6) == 0) {
      is_our_bot = true;
    }
    ```
    The fallback check `strncmp(..., "[FEED]", 6) == 0` causes any newly connecting feeder bot to adopt the snake ID of ANY existing feeder bot whose spawn packet is broadcast by the server upon room entry.
* **Body Targeting Coordinate Calculation**:
  * In `feeder_update` lines 270–291:
    ```c
    int pts_len = tdarray_length(me->pts);
    if (pts_len >= 12) {
      int mid_idx = pts_len / 2;
      target_x = me->pts[mid_idx].xx;
      target_y = me->pts[mid_idx].yy;
    } else if (pts_len >= 4) {
      int safe_idx = pts_len - 1 - 2;
      if (safe_idx < 0) safe_idx = 0;
      target_x = me->pts[safe_idx].xx;
      target_y = me->pts[safe_idx].yy;
    } else if (pts_len > 0) {
      target_x = me->pts[0].xx;
      target_y = me->pts[0].yy;
    } else {
      target_x = me->xx - cosf(me->ang) * 120.0f;
      target_y = me->yy - sinf(me->ang) * 120.0f;
    }
    ```
  * Cross-referencing `app/src/network/callback.c` lines 660–715 and `app/src/game/sbot.c` lines 723–731 confirms: `pts[0]` is the tail, `pts[pts_len - 1]` is behind the head, and `pts[pts_len / 2]` is the geometric midpoint.
* **Turbo Boost Activation**:
  * In `feeder_update` lines 379–384:
    ```c
    bool want_boost = (dist < 2200.0f || dist > 6000.0f);
    if (want_boost != bot->boosted) {
      bot->boosted = want_boost;
      uint8_t cmd = want_boost ? 253 : 254;
      mg_ws_send(bot->c, &cmd, 1, WEBSOCKET_OP_BINARY);
    }
    ```
    Boosting at `dist > 6000.0f` exhausts the feeder bot's mass (spawning at size 10), dropping orbs across empty map regions rather than concentrating mass at the player snake.
* **Death & Respawn Cycle**:
  * Death detected on short `'s'` packet (`cmd == 's' && dlen <= 6 && s_id == bot->snake_id`, lines 111–123) or packet `'v'` (lines 150–161).
  * Sets `bot->alive = false; bot->c->is_closing = true; bot->c = NULL; bot->snake_id = -1; bot->cooldown_until = now + 3.0;`.

---

## 2. Logic Chain

```
[Observation 1.2: B.goal overwritten by evaluate_best_evasion_heading with 32 discrete rays]
                        │
                        ▼
   [Ray angle jumps discontinuously between adjacent 11.25° sectors or opposite quadrants]
                        │
                        ▼
[Observation 1.2: input.c transmits un-smoothed quantized angle sang (0..250) every 50ms]
                        │
                        ▼
   [Snake head jerks violently; S-curve oscillations prevent stable path tracking] ───► JITTER & OSCILLATION CAUSE #1

[Observation 1.2: Rear target pathing flips lead_ang across ±PI via turn_sign]
                        │
                        ▼
   [Instantaneous 198° heading reversal when food is directly behind] ────────────────► JITTER & OSCILLATION CAUSE #2

[Observation 1.2: check_collision treats all s->id != B.id as hostile threats]
                        │
                        ▼
[Observation 1.3: Feeder bots steer straight toward player snake body]
                        │
                        ▼
   [Player bot detects approaching feeder head within 800px, panics, and turns away] ──► FEEDER COLLISION EVASION FAILURE

[Observation 1.2: check_encircle triggers on tiny snakes (length ≤ 6)]
                        │
                        ▼
   [to_circle loop (pn - 6) never matches; blind circular steering executed]
                        │
                        ▼
   [check_collision bypassed in stage == 1 & 2; snake spins into enemies] ─────────────► COILING DEATH TRAP

[Observation 1.3: s_last_bot_spawn initialized to 0.0]
                        │
                        ▼
   [Player connects -> Bot 0 connects simultaneously on same frame]
                        │
                        ▼
   [Slither server triggers per-IP rate limit / TCP reset (code 1006)] ───────────────► INITIAL DISCONNECT / KICKOUT

[Observation 1.3: bot->connecting = false on MG_EV_WS_OPEN, bot->alive remains false until 's']
                        │
                        ▼
   [If challenge '6' or spawn 's' drops, bot has bot->c != NULL, connecting=false, alive=false]
                        │
                        ▼
   [No timeout watchdog exists; slot i never cleared or retried] ─────────────────────► FEEDER RESPAWN LOOP STALL

[Observation 1.3: strncmp(..., "[FEED]", 6) matches any feeder bot]
                        │
                        ▼
   [Connecting bot steals snake_id of already-living peer feeder bot] ────────────────► FEEDER IDENTITY THEFT / DESYNC
```

---

## 3. Caveats

1. **Network Server Latency & Protocol Variance**: Public Slither.io servers (`23.29.125.178:444`) may alter challenge packet formats or rate-limiting thresholds dynamically. Hardcoded `CLIENT_VERSION = 291` and `cwa` signature have been verified against current server traffic, but if the upstream protocol increments its version, handshake responses must match.
2. **Device Performance on Vulkan**: Testing on the target Android physical device (`192.168.7.12:33203`) involves a 60 FPS capped loop with mobile CPU/GPU power throttling. Raycast resolution should not exceed 32–64 sectors to avoid frame-time spikes in `oef.c`.
3. **Head-to-Head Slither Mechanics**: In Slither.io physics, head-to-body collisions are 100% safe for the body owner. However, near the head (`pts_len - 1` to `pts_len - 3`), high-speed lateral motion can accidentally cause head-to-head contact. The targeting index must strictly guarantee a buffer of at least 6 segments behind the head.

---

## 4. Conclusion

The existing bot and feeder implementations contain the core components for Slither protocol communication, but suffer from critical algorithmic and state machine flaws:

1. **Player Bot AI (R1)**:
   * **Root causes of jitter**: Discrete 32-ray snapping without low-pass filtering, goal overwrite conflict between `delay_action` and `evaluate_best_evasion_heading`, and 180° `turn_sign` flipping on flank/rear targets.
   * **Root causes of collision failures**: Small snakes entering defensive coiling blind to collisions, and fleeing from allied feeder bots because they are classified as hostile heads.
   * **Required overhaul**:
     - Replace discrete ray jumping with a continuous angular clearance field with exponential moving average (EMA) smoothing and angular velocity slew-rate limiting (`max_turn_rate * dt`).
     - Filter out allied feeder bots (`s->nk` starting with `"[FEED]"`) from hostile head avoidance.
     - Require minimum score (`≥ 150`) or length (`≥ 30`) before permitting defensive coiling; ensure collision raycasting remains active during coiling to deflect inward.
     - Implement spine-following cluster pursuit for corpse trails and feeder mass drops.

2. **Feeder Swarm (R2)**:
   * **Root causes of TCP reset / 1006**: Simultaneous initial connection race between player and Bot 0 due to uninitialized `s_last_bot_spawn = 0.0`.
   * **Root causes of respawn loop stall**: Lack of explicit connection timeout watchdog (`FEEDER_CONNECTING` / `FEEDER_CHALLENGING` state timeout), leaving `bot->c` allocated and slot `i` frozen indefinitely.
   * **Root causes of identity theft**: Permissive `strncmp(..., "[FEED]", 6)` matching peer bots.
   * **Required overhaul**:
     - Introduce a rigorous 5-state finite state machine (`DISCONNECTED`, `CONNECTING`, `CHALLENGING`, `ACTIVE`, `COOLDOWN`) with a 4.0-second state timeout.
     - Stagger the initial feeder spawn 2.5–3.0 seconds AFTER player match entry.
     - Enforce exact name matching (`bot->name`) for spawn identification.
     - Disable long-range boost (`dist > 6000`), restricting turbo acceleration exclusively to homing impact (`dist < 1800`).

---

## 5. Verification Method

### 5.1 Static Code Inspection
Verify all findings by inspecting the exact locations identified:
* `app/src/game/sbot.c`: lines 480–571 (raycast), 609 (goal overwrite), 677–706 (encircle), 1058–1087 (to_circle), 1171–1178 (turn_sign oscillation), 1350–1362 (loop execution).
* `app/src/game/feeder.c`: lines 86–90 (name matching), 168–172 (connecting state clearing), 319–344 (spawn stagger & state stall), 379–384 (boost conditions).
* `app/src/game/input.c`: lines 36–38, 97–120 (angle quantization and transmission).

### 5.2 Dynamic Physical Device Verification Commands
Once Worker implements the recommended fixes, independently execute:
1. **Compilation & Build**:
   ```bash
   ./gradlew assembleDebug
   ```
2. **Installation to Device**:
   ```bash
   adb install -r app/build/outputs/apk/debug/app-debug.apk
   ```
3. **Automated Verification Script**:
   Run the end-to-end verification harness `python3 verify_v26.py` and inspect:
   * Logcat traces for `feeder_bot` and `yslither_net` showing consecutive `Spawned -> Crashed -> Mass released -> Respawned` cycles.
   * Confirmation of zero TCP reset / code 1006 disconnects over 60 seconds.
   * Screenshots (`v26_monitor_sec_*.png`) showing smooth snake movement, neon green minimap markers, HUD badges (`Tam`, `Ping`, `Coord`, `Bots`), and score growth from `Tam: 10`.

---

## 6. Comprehensive Inventory of Source Artifacts

| Component | File Path | Line Reference | Purpose / Functionality |
|---|---|---|---|
| **Bot Header** | `app/src/game/sbot.h` | 10–23 | Output struct (`xm`, `ym`, `accel`) & lifecycle exports |
| **Bot State** | `app/src/game/sbot.c` | 79–120 | State definitions, arrays for angles, collisions, food |
| **Bot Collision Scan** | `app/src/game/sbot.c` | 395–466 | `get_collision_points()`: head projections, body sampling, rim |
| **Bot Ray Evaluator** | `app/src/game/sbot.c` | 469–610 | `evaluate_best_evasion_heading()`: 32-sector raycast clearance |
| **Bot Threat Trigger** | `app/src/game/sbot.c` | 612–675 | `check_collision()`: immediate threat detection & evasion lock |
| **Bot Encircle Logic** | `app/src/game/sbot.c` | 677–706 | `check_encircle()`: angular coverage threshold |
| **Bot Coiling** | `app/src/game/sbot.c` | 892–1087 | `follow_circle_self()` & `to_circle()` |
| **Bot Food Scoring** | `app/src/game/sbot.c` | 289–388, 1089–1129 | `add_food_angle()` & `compute_food_goal()` |
| **Bot Main Step** | `app/src/game/sbot.c` | 1295–1431 | `sbot_go()`: mode dispatch, goal setting, output assignment |
| **Feeder Header** | `app/src/game/feeder.h` | 9–32 | `MAX_FEEDER_BOTS 4`, `feeder_bot_pos`, public API |
| **Feeder State & WS** | `app/src/game/feeder.c` | 25–48, 164–216 | `feeder_bot` struct, Mongoose manager, `feeder_ws_cb` |
| **Feeder Packets** | `app/src/game/feeder.c` | 50–162 | `feeder_bot_on_packet()`: challenge '6', 's', '=', '+', 'v' |
| **Feeder Update Loop**| `app/src/game/feeder.c` | 234–402 | `feeder_update()`: stagger, body tracking, boost, poll |
| **Input Angle Send** | `app/src/game/input.c` | 36–38, 97–120 | Maps bot output to `atan2f(ym, xm)` and sends `sang` |
| **Network Protocol** | `app/src/network/callback.c` | 30–85, 122–181 | `decode_secret()`, challenge response, spawn assembly |
| **Main Game Loop** | `app/src/game/loop.c` | 73–83, 96–120 | Frame orchestration, disconnect recovery, respawn |
| **HUD & Badges** | `app/src/game/ui_overlay.c` | 10–182, 184–240 | Top status badges & minimap neon green feeder markers |
| **Touch Controls** | `app/src/game/custom_controls.c`| 106–128, 217–259 | On-screen touch buttons for BOT and BOTS |

---

## 7. Concrete Architectural Recommendations for Worker

### Recommendation 1: Player Bot Steering Smoother & Slew-Rate Limiter (R1)
* **Contract**:
  ```c
  typedef struct {
    float wanted_angle;      // Current filtered target angle (radians)
    float current_turn_rate; // Current angular velocity
    int lock_turn_dir;       // -1 = CCW, +1 = CW, 0 = neutral
    double lock_until;       // Timestamp preventing direction flip
  } bot_steering_filter;
  ```
* **Algorithm**:
  1. Evaluate raycast clearance across 32 or 64 sectors. Score each sector with continuity penalty:
     $$\text{Penalty}(\theta) = \exp\left(-\frac{(\theta - \theta_{\text{prev}})^2}{2\sigma^2}\right)$$
  2. Compute optimal target angle $\theta^*$.
  3. Apply angular slew-rate clamp:
     $$\Delta\theta = \text{ang\_between}(\theta^*, \theta_{\text{prev}})$$
     $$\theta_{\text{new}} = \theta_{\text{prev}} + \text{clamp}(\Delta\theta, -\omega_{\max}\Delta t, +\omega_{\max}\Delta t)$$
  4. Write `dx = cosf(theta_new) * 500.0f; dy = sinf(theta_new) * 500.0f;`.

### Recommendation 2: Feeder Bot Immunity in Player Bot Threat Evaluator (R1)
* In `app/src/game/sbot.c` line 403 (`get_collision_points`):
  ```c
  // Exclude allied feeder bots from hostile enemy obstacle scanning
  if (strncmp(s->nk, "[FEED]", 6) == 0) continue;
  ```
  This immediately stops the player snake from evading feeder bots homing into its body.

### Recommendation 3: Robust Feeder Finite State Machine with Timeouts (R2)
* **Contract**:
  ```c
  typedef enum {
    FEEDER_STATE_DISCONNECTED = 0,
    FEEDER_STATE_CONNECTING,
    FEEDER_STATE_CHALLENGING,
    FEEDER_STATE_ACTIVE,
    FEEDER_STATE_COOLDOWN
  } feeder_state_t;
  ```
* **State Machine Invariants**:
  - `FEEDER_STATE_CONNECTING`: Triggered on `mg_ws_connect`. Timeout: 4.0s. If exceeded, force `bot->c->is_closing = true; bot->c = NULL;` and transition to `FEEDER_STATE_COOLDOWN`.
  - `FEEDER_STATE_CHALLENGING`: Entered upon `MG_EV_WS_OPEN`. Timeout: 4.0s. If challenge '6' or spawn 's' not completed, transition to `COOLDOWN`.
  - `FEEDER_STATE_ACTIVE`: Entered upon receiving own `'s'` spawn packet. Active dead-reckoning, 50ms body-homing steering, 1.2s keepalive pings.
  - `FEEDER_STATE_COOLDOWN`: Duration 3.0s. Clean socket teardown. Guaranteed slot recycling.

### Recommendation 4: Staggered Spawn Sequencing (R2)
* In `feeder_init` or whenever `gdata->conn` switches to `CONNECTED`:
  ```c
  s_last_bot_spawn = glfwGetTime() + 1.5; // Delay first feeder by 1.5s after player joins
  ```
  Ensure subsequent bots spawn at `s_last_bot_spawn + 3.0`.

### Recommendation 5: Close-Range-Only Homing Turbo (R2)
* Update boost condition in `feeder_update`:
  ```c
  bool want_boost = (dist < 1800.0f && dist > 150.0f);
  ```
  This preserves 100% mass during approach across the arena, unleashing full velocity only for the final suicide body strike.
