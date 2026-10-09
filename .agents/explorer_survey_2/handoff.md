# Handoff Report: Network Protocol & Server Connection Stability (Requirement R3)

**Author:** Network Protocol Explorer (Survey 2)  
**Date:** 2026-09-22T16:45:00Z  
**Target:** Parent Orchestrator (`b673f1a9-0adf-43e6-bf42-cddae60d4e2d`) and Downstream Worker Implementers  
**Working Directory:** `/root/.gemini/antigravity-cli/scratch/yslither/.agents/explorer_survey_2`  
**Target Codebase:** `/root/.gemini/antigravity-cli/scratch/yslither`

---

## 1. Observation

### 1.1 Network Stack & Libraries
- **WebSocket Library:** Mongoose Embedded Networking Library v7.20 (`app/src/external/mongoose.h:23`, `app/src/external/mongoose.c`).
- **Connection Context:**
  - `gdata->network_manager` (`struct mg_mgr`, `app/src/game/game_data.h:54`).
  - `gdata->connection` (`struct mg_connection*`, `app/src/game/game_data.h:55`).
  - Separate manager for feeder bots: `s_feeder_mgr` (`struct mg_mgr`, `app/src/game/feeder.c:42`).
  - Background server latency checker: raw POSIX BSD TCP socket `connect()` on port 444 (`app/src/network/server_list.c:66-96`).
- **Thread Model:**
  - The main game and networking loop runs **single-threaded on the Android Native thread / Vulkan Render thread** (`android_main.c:326-382`).
  - Network polling is driven synchronously via `server_poll(env)` which calls `mg_mgr_poll(&gdata->network_manager, 0)` with timeout 0 (`app/src/network/server.c:49-54`, `app/src/game/loop.c:42, 78`).
  - A detached background POSIX thread `ping_worker_thread` is spawned in `server_list.c:161` via `pthread_create(&s_ping_thread, NULL, ping_worker_thread, NULL)`.

### 1.2 "JUGAR" Tap and Match Entry Flow
- **UI Button Definition:** `app/src/ui/title_screen.c:527-543`:
  ```c
  if (igButton("\uea1c  J U G A R", (ImVec2){menu_w, 64.0f})) {
    usr->gdata.conn = CONNECTING;
    usr->gdata.curr_screen = PLAYING;
    usr->gdata.connect_retry_count = 0;
    glfwSetTime(0);

    // If server is invalid or empty or obsolete default or unreachable, select best ping server automatically
    if (usrs->ipv4[0] == '\0' || strcmp(usrs->ipv4, "192.211.52.146:444") == 0 || server_list_get_ping_by_ip(usrs->ipv4) == 999) {
      const char* best = server_list_get_best_ip();
      if (best && best[0] != '\0') {
        strncpy(usrs->ipv4, best, MAX_IPV4_LEN);
      }
    }

    usrs->hotkeys[HOTKEY_BOT].active = usrs->bot_auto_start; // False by default
    server_connect(env);
  }
  ```
- **Connection Initiation:** `app/src/network/server.c:23-38`:
  ```c
  void server_connect(tenv* env) {
    tuser_data* usr = env->usr;
    game_data* gdata = &usr->gdata;
    user_settings* usrs = &usr->usrs;

    char url[256] = {};
    sprintf(url, "ws://%s/slither", usrs->ipv4);
    LOGI("server_connect: Attempting WebSocket connection to %s", url);
    gdata->connection =
      mg_ws_connect(&gdata->network_manager, url, server_callback, env,
                    "%s:%s\r\n",
                    "Origin", "https://slither.com");
    if (!gdata->connection) {
      LOGE("server_connect: Failed to allocate connection object!");
    }
  }
  ```
- **State Transition to PLAYING:** In `app/src/main.c:85-94`:
  ```c
  switch (usr->gdata.curr_screen) {
    case TITLE_SCREEN:
      ui_title_screen(env);
      break;
    ...
    case PLAYING:
      game_loop(env);
      break;
  }
  ```
- **Connecting Phase Execution:** In `app/src/game/loop.c:30-71`:
  Evaluates `gdata->conn == CONNECTING`:
  - Enforces 3.5s timeout: `if (cur_t > 3.5) { if (gdata->connection) gdata->connection->is_closing = true; ... }`
  - Polls network: `server_poll(env);`
  - Renders progress bar: `igProgressBar(-glfwGetTime(), ...);`
  - When `gdata->closed` is set: executes failover attempt up to 3 times via `server_list_get_fallback_ip()`.

### 1.3 Protocol Handshake & Packet Processing
- **WebSocket Handshake (`MG_EV_WS_OPEN`):** `app/src/network/callback.c:1330-1334`:
  ```c
  mg_ws_send(c, (uint8_t[]){1}, 1, WEBSOCKET_OP_BINARY);
  mg_ws_send(c, (uint8_t[]){'c', 0}, 2, WEBSOCKET_OP_BINARY);
  ```
- **Server Challenge Packet `'6'`:** `app/src/network/callback.c:122-181`:
  - Solves challenge using `decode_secret(a, a_len, secret)`.
  - Sends 27-byte secret: `mg_ws_send(c, secret, 27, WEBSOCKET_OP_BINARY);`.
  - Constructs spawn request packet:
    - Byte 0: `115` (`'s'`)
    - Byte 1: `30` (protocol subversion)
    - Bytes 2-3: `CLIENT_VERSION` (291)
    - Bytes 4-23: 20 authentication bytes `cwa[20] = {54, 206, 204, 169, 97, 178, 74, 136, 124, 117, 14, 210, 106, 236, 8, 208, 136, 213, 140, 111}`
    - Byte 24: `default_skin`
    - Byte 25: `nick_len`
    - Bytes 26..: `nickname` string
    - Byte `m++`: `0`
    - Byte `m++`: `accessory`
    - Optional custom skin segment (8 header bytes + compressed skin codes).
    - Sends spawn packet: `mg_ws_send(c, ba, m, WEBSOCKET_OP_BINARY);`.
- **Arena Setup Packet `'a'`:** `app/src/network/callback.c:182-230`:
  - Parses `grd` (arena radius), `sector_size`, `spangdv`, `nsp1..3` (speed multipliers), `mamu`, `mamu2`, `cst`, `protocol_version`, `default_msl`, `flux_grd`.
  - Calls `recalc_sep_mults(gdata)` and `set_mscps(gdata, nmscps)`.
- **Snake Spawn Packet `'s'` (`dlen > 6`):** `app/src/network/callback.c:231-423`:
  - Parses snake ID, angle, turning direction, target angle, speed, fam, skin, coordinates `(snx, sny)`, nickname, body parts list.
  - Matches player snake on lines 363-391:
    ```c
    if (gdata->conn == CONNECTING) {
      usr->r->global.lview[0] = gdata->data.lview_xx;
      usr->r->global.lview[1] = gdata->data.lview_yy;

      gdata->data.snake_id = id;
      gdata->data.dead = false;
      gdata->data.follow_view = true;

      gdata->data.view_xx = xx;
      gdata->data.view_yy = yy;
      ...
      gdata->conn = CONNECTED;
      LOGI("Snake spawned successfully! Game is now CONNECTED!");
      glfwSetTime(0);
    }
    ```

### 1.4 Disconnection & Death Handling
- **Packet `'v'` (Player Death):** `app/src/network/callback.c:1299-1320`:
  Sets `gdata->data.dead = true; death_time = glfwGetTime(); follow_view = false;` and saves user score/stats.
- **Packet `'s'` (`dlen <= 6`, Snake Death/Removal):** `app/src/network/callback.c:424-457`:
  If `o->id == gdata->data.snake_id`, sets `gdata->data.snake_id = -1; gdata->data.dead = true; death_time = glfwGetTime();`.
- **Connection Close (`MG_EV_CLOSE`):** `app/src/network/callback.c:1363-1368`:
  ```c
  if (c == gdata->connection) {
    gdata->closed = true;
    gdata->connection = NULL;
  }
  ```
- **In-Game Death / Drop Loop Recovery:** `app/src/game/loop.c:100-128`:
  - If `dead` and bot mode / instant restart is active: after 1.2s delay, calls `server_disconnect(env); game_data_reset(env); usr->gdata.conn = CONNECTING; glfwSetTime(0); server_connect(env);`.
  - If `closed` while alive (`!gdata->data.dead`): calls `server_disconnect(env); game_data_reset(env); usr->gdata.conn = CONNECTING; glfwSetTime(0); server_connect(env);`.
  - If `dead` and manual player: displays Game Over dialog with "REAPARECER" and "Salir al Menu" (`ui_overlay.c:560-587`).

---

## 2. Logic Chain

### 2.1 Why Connection Hangs Occur on the Loading Screen
1. In `app/src/game/loop.c:37-40`:
   ```c
   double cur_t = glfwGetTime();
   if (cur_t > 3.5) { // 3.5 sec fast timeout per server attempt
     if (gdata->connection) gdata->connection->is_closing = true;
     LOGE("Connection timed out after %.2f seconds, triggering failover...", cur_t);
   }
   ```
2. The failover trigger on lines 54-70 strictly depends on `if (gdata->closed)`.
3. If `mg_ws_connect()` fails synchronously (returns `NULL` due to allocation or network bind error) or if `c->is_closing = true` does not fire `MG_EV_CLOSE` (e.g. stalled socket state before connect event), `gdata->connection` is `NULL`.
4. Because `gdata->connection` is `NULL`, `gdata->connection->is_closing = true` is never executed, and `gdata->closed` is never set to `true`.
5. Consequently, `if (gdata->closed)` never evaluates to true. `cur_t` increases past 3.5 seconds indefinitely, printing `LOGE` on every frame while the client remains stuck on the progress bar forever.

### 2.2 Why Stale Sockets & Desync Occur (Dirty Reconnection Lifecycle)
1. In `title_screen.c:542` (when tapping JUGAR) and in `loop.c:64` (during server failover retry), `server_connect(env)` is called directly without invoking `server_disconnect(env)`.
2. In `app/src/network/server.c:23-38`, `server_connect` calls `mg_ws_connect(&gdata->network_manager, ...)` without clearing previous connections.
3. The previous connection object remains allocated inside `gdata->network_manager.conns`.
4. When `gdata->closed` is handled in `loop.c:54-70`, failover advances `gdata->connect_retry_count` and calls `server_connect(env)` again while the failed connection's socket is still draining or pending closure.
5. If failover retries are exhausted (`connect_retry_count >= 3`), `loop.c:68` sets `gdata->conn = DISCONNECTED`, but **never calls `server_disconnect(env)`**.
6. When the user returns to the title screen and taps JUGAR again, `gdata->network_manager` still contains lingering, half-closed connection structures.

### 2.3 Why Data Races and Memory Corruption Occur in Server Selection
1. `app/src/network/server_list.c:161` spawns `ping_worker_thread` as a detached thread (`pthread_detach(s_ping_thread)`).
2. Inside `ping_worker_thread` (`server_list.c:126`):
   ```c
   pthread_mutex_lock(&s_mutex);
   qsort(s_servers, s_server_count, sizeof(server_entry), server_ping_cmp);
   s_is_pinging = false;
   pthread_mutex_unlock(&s_mutex);
   ```
3. Meanwhile, on the main render thread, `ui_server_selector()` in `app/src/ui/title_screen.c:246` calls:
   ```c
   server_entry* s = server_list_get(i);
   ```
4. `server_list_get(int index)` in `server_list.c:144-147`:
   ```c
   server_entry* server_list_get(int index) {
     if (index < 0 || index >= s_server_count) return NULL;
     return &s_servers[index];
   }
   ```
5. `server_list_get` **does not acquire `s_mutex`**. Returning a direct pointer `&s_servers[index]` to memory being simultaneously reordered by `qsort` causes torn reads, corrupted string pointers for `s->name`/`s->ip`, and intermittent crashes.

### 2.4 Why Player Snake Identity Can Be Hijacked
1. In `app/src/network/callback.c:363`:
   ```c
   if (gdata->conn == CONNECTING) {
     gdata->data.snake_id = id;
     ...
     gdata->conn = CONNECTED;
   }
   ```
2. The client assumes the first snake packet `'s'` received while `conn == CONNECTING` belongs to the player.
3. If an existing nearby snake or feeder bot packet arrives prior to the player's own spawned snake, `gdata->data.snake_id` is assigned the foreign snake's ID.
4. When that foreign snake dies or leaves the sector, `callback.c:432` matches `o->id == gdata->data.snake_id`, triggers player death (`gdata->data.dead = true`), and prematurely boots the player or pops the death dialog while the real player snake is still alive in the arena.

### 2.5 Why Packet Decoding Causes Out-of-Bounds Memory Access
1. In `app/src/network/callback.c:1340-1353`:
   ```c
   while (m < l) {
     int len;
     if (a[m] < 32) {
       len = a[m] << 8 | a[m + 1];
       m += 2;
     } else {
       len = a[m] - 32;
       m++;
     }
     uint8_t* a2 = a + m;
     got_packet(env, a2, len);
     m += len;
   }
   ```
2. If `a[m] < 32` occurs at `m == l - 1`, reading `a[m + 1]` is an out-of-bounds read past the buffer.
3. If `m + len > l`, `got_packet()` receives a `len` extending past the end of `msg->data.len`.
4. Subsequent parsing in `got_packet()` (such as snake body parsing or nickname copying in `callback.c:257`) reads garbage memory or causes a `SIGSEGV`.
5. In `feeder.c:183, 190`, boundary guards (`if (m + 1 >= total_len) break;` and `if (m + sub_len > total_len) break;`) were added, but they were never ported to `callback.c`.

### 2.6 Null Pointer Dereference in Hotkeys
1. In `app/src/game/loop.c:88, 92`:
   ```c
   if (usrs->hotkeys[HOTKEY_QUIT].active || ...) {
     gdata->connection->is_closing = true;
   } else if (usrs->hotkeys[HOTKEY_RESTART].active || ...) {
     gdata->connection->is_closing = true;
     gdata->restart_req = true;
   }
   ```
2. When a connection drops and `server_callback` sets `gdata->connection = NULL;`, if the user activates hotkeys or clicks middle/right mouse buttons before reconnection finishes, `gdata->connection` is dereferenced while `NULL`, triggering an immediate crash (`SIGSEGV`).

---

## 3. Caveats

1. **Server Rate Limiting (Anti-Bot / Code 1006):** Official Slither.io servers implement undocumented per-IP connection rate limiting. While client code can enforce a 3.0s connection stagger between feeder bots and the player, server-side threshold changes could still trigger TCP resets if too many bots share one external IP.
2. **Local Ping vs Remote Ping:** Ping measurements in `server_list.c` use TCP connect latency to port 444, whereas in-game latency measures application-level WebSocket ping packet `251` to pong `'p'`. TCP ping gives relative server ranking but not exact in-game latency.
3. **Mongoose Synchronous DNS:** Mongoose's `mg_ws_connect()` executes synchronously. Because `usrs->ipv4` stores dotted decimal IP addresses (`23.29.125.178:444`), DNS resolution is bypassed. If domain names are ever introduced, non-blocking asynchronous DNS or pre-resolution would be required to prevent render thread hitches.

---

## 4. Conclusion

The network protocol architecture is built upon Mongoose v7.20 operating synchronously within the single-threaded Vulkan render loop. Connection instability and match entry failures stem from five concrete defects:
1. **Hang in `CONNECTING`:** A 3.5s timeout that assumes `gdata->connection` is non-null and that setting `is_closing = true` always triggers `gdata->closed`. When `gdata->connection` is null, failover never triggers and the client hangs on the progress bar forever.
2. **Dirty Connection Lifecycle:** Repeated calls to `server_connect()` without calling `server_disconnect()` during failover and title screen transitions, leaking connections inside Mongoose.
3. **Data Race in Server List:** Unsynchronized access to `s_servers` in `server_list_get()` while the detached background thread `ping_worker_thread` executes `qsort()`.
4. **Player Snake Identity Ambiguity:** Blind assignment of `gdata->data.snake_id` to the first received snake packet `'s'` without matching `usrs->nickname`, causing camera hijacking and premature death triggers.
5. **Sub-packet Buffer Over-read:** Missing length validation in `callback.c:1340-1353` for bundled WebSocket sub-packets.

---

## 5. Verification Method & Actionable Recommendations

### 5.1 Architecture & Interface Contract for Worker Implementation

#### Contract 1: Unified Connection State Machine (`server.h` / `server.c`)
Provide an atomic connection management interface that guarantees clean lifecycle transitions:
```c
// Ensures any existing connections and managers are cleaned up before connecting
void server_connect_clean(tenv* env, const char* target_ip);

// Complete disconnection and manager reinitialization
void server_disconnect_clean(tenv* env);

// Single failover helper that forces socket closure, cleans up, and attempts next server
bool server_failover_next(tenv* env);
```

#### Contract 2: Resilient Timeout & Failover in `loop.c`
In `app/src/game/loop.c:37-70`:
```c
double cur_t = glfwGetTime();
if (cur_t > 3.5) {
  LOGE("Connection attempt timed out after %.2f seconds, forcing failover...", cur_t);
  if (gdata->connection) {
    gdata->connection->is_closing = true;
  }
  gdata->closed = true; // Force closed flag so failover logic is guaranteed to execute
}

server_poll(env);

if (gdata->closed) {
  gdata->closed = false;
  if (gdata->connect_retry_count < 3) {
    gdata->connect_retry_count++;
    const char* next_ip = server_list_get_fallback_ip(gdata->connect_retry_count);
    if (next_ip && next_ip[0] != '\0') {
      LOGI("Failover: Switching to alternative server %s (attempt %d/3)...", next_ip, gdata->connect_retry_count);
      strncpy(usrs->ipv4, next_ip, MAX_IPV4_LEN);
      server_disconnect(env);
      game_data_reset(env);
      gdata->conn = CONNECTING;
      glfwSetTime(0);
      server_connect(env);
      break;
    }
  }
  // All retries exhausted: reset cleanly to title screen
  LOGE("All connection attempts failed. Returning cleanly to title screen.");
  server_disconnect(env);
  game_data_reset(env);
  gdata->conn = DISCONNECTED;
  gdata->curr_screen = TITLE_SCREEN;
  gdata->connect_retry_count = 0;
}
```

#### Contract 3: Thread-Safe Server List Access (`server_list.h` / `server_list.c`)
Replace the raw pointer return with mutex-protected value copying:
```c
// In server_list.h:
bool server_list_get_copy(int index, server_entry* out_entry);

// In server_list.c:
bool server_list_get_copy(int index, server_entry* out_entry) {
  if (!out_entry || index < 0 || index >= s_server_count) return false;
  pthread_mutex_lock(&s_mutex);
  *out_entry = s_servers[index];
  pthread_mutex_unlock(&s_mutex);
  return true;
}
```

#### Contract 4: Safe Packet Sub-bundling in `callback.c`
In `app/src/network/callback.c:1340-1358`:
```c
if (a[m] < 32) {
  int l = msg->data.len;
  while (m < l) {
    int len;
    if (a[m] < 32) {
      if (m + 1 >= l) break; // BOUNDS CHECK
      len = a[m] << 8 | a[m + 1];
      m += 2;
    } else {
      len = a[m] - 32;
      m++;
    }
    if (m + len > l) break; // BOUNDS CHECK
    uint8_t* a2 = a + m;
    got_packet(env, a2, len);
    m += len;
  }
}
```

#### Contract 5: Match Nickname on Player Snake Spawn
In `app/src/network/callback.c:363`:
```c
// Verify that the snake belongs to the player (matches nickname or is not a feeder bot)
bool is_feeder = (strncmp(o.nk, "[FEED]", 6) == 0);
bool name_matches = (usrs->nickname[0] != '\0' && strcmp(o.nk, usrs->nickname) == 0);

if (gdata->conn == CONNECTING && !is_feeder && (name_matches || usrs->nickname[0] == '\0')) {
  // player snake initialization
  ...
}
```

#### Contract 6: NULL Checks on Hotkeys in `loop.c:85-95`
```c
if (usrs->hotkeys[HOTKEY_QUIT].active || ...) {
  if (gdata->connection) gdata->connection->is_closing = true;
} else if (usrs->hotkeys[HOTKEY_RESTART].active || ...) {
  if (gdata->connection) {
    gdata->connection->is_closing = true;
    gdata->restart_req = true;
  }
}
```

---

### 5.2 Independent Verification Procedure
1. **Compilation & APK Build:**
   ```bash
   cd /root/.gemini/antigravity-cli/scratch/yslither
   ./gradlew assembleDebug
   ```
2. **Device Installation & Execution:**
   ```bash
   adb install -r app/build/outputs/apk/debug/app-debug.apk
   adb shell am start -n com.yslither.game/android.app.NativeActivity
   ```
3. **Logcat Monitoring:**
   Verify connection handshake, challenge resolution, and spawn sequence:
   ```bash
   adb logcat -c
   adb logcat -s yslither yslither_net yslither_loop feeder_bot
   ```
   *Expected signature:*
   - `server_connect: Attempting WebSocket connection to ws://...`
   - `server_callback: Connection opened`
   - `server_callback: WebSocket handshake established! Sending init bytes`
   - `got_packet: Received server challenge packet '6', responding to spawn snake...`
   - `Snake spawned successfully! Game is now CONNECTED!`
4. **Stress Testing Connection Failover:**
   - Temporarily set `usrs->ipv4` to an unreachable IP (e.g. `10.255.255.1:444`).
   - Tap "JUGAR".
   - Verify that failover cycles through alternative servers at 3.5s intervals and cleanly returns to `TITLE_SCREEN` on retry exhaustion without freezing the UI or crashing.
5. **Multi-bot Feeding Stability:**
   - Activate feeder bots (`BOTS` HUD button).
   - Verify zero code 1006 drops or per-IP rate-limit kicks during 60+ seconds of concurrent gameplay.
