# Original User Request

## 2026-09-22T16:36:06Z

Comprehensive overhaul, bug remediation, and stability verification of the yslither Vulkan Android game client, focusing on autonomous player bot intelligence, feeder bots match entry and suicide collision lifecycle, game server connection reliability, and minimap rendering accuracy.

Working directory: `/root/.gemini/antigravity-cli/scratch/yslither`
Integrity mode: development

## Requirements

### R1. Autonomous Player Bot AI Overhaul
Implement a robust, highly capable autonomous snake AI inspired by proven Slither.io bot algorithms. The bot must:
- Navigate smoothly without jitter or rapid 180-degree oscillations.
- Prioritize high-value food orbs, corpse trails, and feeder bot drops with proactive pathing.
- Perform 360-degree predictive obstacle avoidance (repelling from enemy bodies, aggressively dodging enemy heads, and maintaining clearance from arena rims).
- Execute defensive coiling/circling maneuvers when surrounded or trapped by larger snakes.

### R2. Feeder Bots Match Entry and Feeding Cycle
Fix and guarantee the full lifecycle of feeder bots:
- Bots must reliably establish WebSocket connections and solve server challenge packets without dropouts or handshake failures.
- Connection attempts must respect server per-IP rate limits with staggered intervals to prevent socket drops (TCP reset / code 1006).
- Bots must continuously track the player snake's body coordinates and home directly into the middle body segments (`me->pts[pts_len / 2]`), accelerating with turbo to crash into the body and die, releasing all mass for the player to feast on.
- When destroyed, bots must cleanly close connections and respawn in a smooth, continuous loop.

### R3. Server Connection and Match Entry Stability
Eliminate match connection errors and premature disconnects:
- Seamless game joining upon tapping "JUGAR", with clean state initialization and handshake handling.
- Graceful recovery and cleanup during server disconnects or player death, preventing hangs or unintended kickouts to the title screen while alive.

### R4. Minimap and HUD Rendering Precision
Fix all visual and coordinate calculation bugs on the minimap and top HUD:
- Exact 1:1 mapping of world coordinates `(x, y)` to circular minimap coordinates, matching the arena flux boundary without drift.
- Render player marker, feeder bots (vibrant neon green with directional homing lines), and enemy snakes with accurate positions.
- Display real-time status badges for score/size (`Tam`), ping ms, world coordinates, and feeder bot count/distance without text clipping or layout overlap.

## Acceptance Criteria

### Player Bot Autonomous Performance
- [ ] Player snake survives continuously under bot control for at least 60 seconds without colliding into obstacles.
- [ ] Snake score/size increases measurably from initial spawn (`Tam: 10`) through active food seeking.
- [ ] Bot avoids enemy snake heads by maintaining a minimum safe clearance buffer of at least 500 units.

### Feeder Bots Match Entry and Feeding
- [ ] At least 2 feeder bots connect, solve challenge, and spawn into the game server concurrently.
- [ ] Feeder bots steer directly toward the player's body and crash, triggering death packet `'v'` or `'s'` without damaging the player snake.
- [ ] Feeder bots respawn automatically following a clean cooldown, providing continuous feeding mass.
- [ ] Zero server rate-limit kicks or IP disconnections during multi-bot operation.

### Match Entry and Minimap
- [ ] Tapping "JUGAR" reliably connects to the server and enters the match without requiring manual server reselection.
- [ ] Minimap displays real-time markers for the player and feeder bots with correct relative positions and guide lines.
- [ ] Top HUD accurately reflects feeder bot count and proximity distance in real time.

### Physical Device Verification
- [ ] Clean build and installation of the updated APK on the connected Android device (`192.168.7.12:33203`).
- [ ] Programmatic end-to-end test execution with logcat traces and gameplay screenshots proving all criteria are satisfied.
