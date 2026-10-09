# Handoff Report: Renderer (Vulkan/ImGui), Minimap/HUD Precision (R4), and Physical Device Verification

**Author**: Explorer Survey 3 (Renderer & Physical Device)  
**Date**: 2026-09-22T16:46:30Z  
**Target Milestone**: Survey & Codebase Investigation (R4 Minimap/HUD & Physical Device)  
**Status**: Complete (Hard Handoff)  

---

## 1. Observation

### 1.1 Rendering Pipeline Architecture (Vulkan & Dear ImGui)
- **Vulkan Initialization**: Located in `thermite/src/graphics/tcontext.c:28-200`. Context creation queries physical devices and presentation modes.
  - Present Mode: At line 159, requires `scores[i].supports_fifo`. Lines 296-301:
    ```c
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    #ifndef __ANDROID__
    if (!vsync) {
      present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
    #endif
    ```
  - Surface Transform & Dimensions: Lines 244-248 and 264-270:
    ```c
    #ifdef __ANDROID__
    if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
      pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    ...
    if (context->size[0] > context->size[1] && swapchain_extent.width < swapchain_extent.height) {
      uint32_t tmp = swapchain_extent.width;
      swapchain_extent.width = swapchain_extent.height;
      swapchain_extent.height = tmp;
    }
    #endif
    ```
  - Composite Alpha: Lines 287-294 checks `VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR` and falls back to `INHERIT` or `PRE_MULTIPLIED`.
- **Render Loop**: Located in `app/src/rendering/renderer.c:332-373` (`renderer_render`):
  - Line 340: `mm_renderer_prerender(r->mmr, ctx)` transfers CPU density map buffer to GPU texture via `vkCmdCopyBufferToImage`.
  - Line 356: binds global unit quad buffer `r->quad_buffer` (`{{0,0}, {0,1}, {1,0}, {1,1}}`).
  - Lines 364-371: draws passes in order: `bg_renderer_render` (background grid), `fd_renderer_render` (food), `bst_renderer_render` (boost bottom), `bp_renderer_render` (snake bodies), `bst_renderer_render` (boost top), `bd_renderer_render` (arena flux boundary), `bp_renderer_render` (snake eyes/accessories), and `mm_renderer_render` (minimap texture quad).
- **Dear ImGui Overlay Pass**: Located in `app/src/imgui_setup.c` and `app/src/game/ui_overlay.c`:
  - `imgui_init` initializes Vulkan backend (`igImplVulkan_Init`) and sets fallback display size `(1920, 1080)` or window size (`1600, 720`).
  - `android_main.c:166-177` maps `AMotionEvent` to `io->MousePos` and `io->MouseDown[0]`.
  - `ui_overlay.c:275-591` executes Dear ImGui UI passes: `render_stats_hud`, `render_minimap_bots_overlay`, `sbot_render_overlay`, `custom_controls_render_hud`.

---

### 1.2 Minimap World-to-Screen Mapping & Boundary Misalignment (R4)
- **Minimap Geometry & Positioning**:
  - `app/src/game/ui_overlay.c:467-474`:
    ```c
    usr->r->global.minimap_circ[2] = usrs->minimap_size;
    usr->r->global.minimap_circ[0] =
        ctx->size[0] - usr->r->global.minimap_circ[2] - style->WindowPadding.x;
    usr->r->global.minimap_circ[1] = ctx->size[1] -
                                     usr->r->global.minimap_circ[2] -
                                     style->WindowPadding.y - line_height;
    usr->r->global.minimap_opacity = 1;
    ```
- **Vulkan Minimap Shader (`app/res/shaders/src/mm.slang`)**:
  - Vertex shader lines 22-29:
    ```slang
    float2 p = (input.pos - 0.5) * (global.minimap_circ.z + EDGE_SOFTNESS);
    float2 r = p + 0.5 * (global.minimap_circ.z + EDGE_SOFTNESS * 0.5);
    float2 hv = global.viewport * 0.5;
    float2 wp = r + global.minimap_circ.xy;
    float2 o = (wp - hv) / hv;
    output.clipspace = float4(o, 1);
    ```
  - Player Dot in Shader (lines 32-35):
    ```slang
    output.uvp = ((global.view - global.grd) / global.bd_radius + 1.0f) * 0.5f;
    output.uvp = output.uvp * 2 - 1;
    output.uvp *= SHADOW; // SHADOW = 0.9
    output.uvp = output.uvp * 0.5 + 0.5;
    ```
  - Arena Boundary in Shader (lines 17, 64-74):
    - `static const float SHADOW = 0.9;`
    - Outer boundary rim is rendered between `d = SHADOW (0.9)` and `d = 1.0` with color `sc = float4(lerp(global.bd_color, float3(1, 1, 1), 0.25), 1.75) * s;`.
    - Shader player marker (`pdc`) drawn at `input.uvp` in pink (`float4(1, 0.5, 0.5, 1) * ap;`) with black outline (`dc`).
  - Shader Global Uniform Assignment (`app/src/game/redraw.c:1503-1507`):
    ```c
    usr->r->global.view[0] = gdata->data.view_xx;
    usr->r->global.view[1] = gdata->data.view_yy;
    usr->r->global.zoom = gdata->data.gsc;
    usr->r->global.grd = gdata->data.grd;
    usr->r->global.bd_radius = gdata->data.flux_grd;
    ```
- **Minimap Overlay in ImGui (`app/src/game/ui_overlay.c:184-273`)**:
  - Lines 193-210:
    ```c
    float mm_cx = mm_x + mm_size * 0.5f;
    float mm_cy = mm_y + mm_size * 0.5f;
    float mm_rad = mm_size * 0.5f * 0.90f; // 0.90 SHADOW radius matches mm.slang
    float arena_r = (gdata->data.grd > 0.0f) ? gdata->data.grd : 21600.0f;
    ...
    float p_scr_x = mm_cx;
    float p_scr_y = mm_cy;
    if (me) {
      float p_nx = (me->xx - arena_r) / arena_r;
      float p_ny = (me->yy - arena_r) / arena_r;
      p_scr_x = mm_cx + p_nx * mm_rad;
      p_scr_y = mm_cy + p_ny * mm_rad;
    }
    ```
  - Feeder Bots Loop (lines 218-239):
    ```c
    float b_nx = (fbots[i].x - arena_r) / arena_r;
    float b_ny = (fbots[i].y - arena_r) / arena_r;
    float bx = mm_cx + b_nx * mm_rad;
    float by = mm_cy + b_ny * mm_rad;
    ...
    if (me) {
      ImDrawList_AddLine(dl, (ImVec2){bx, by}, (ImVec2){p_scr_x, p_scr_y}, 0x5500FF66, 1.2f);
    }
    ImDrawList_AddCircleFilled(dl, (ImVec2){bx, by}, pulse, 0x4400FF44, 16);
    ImDrawList_AddCircleFilled(dl, (ImVec2){bx, by}, 3.8f, 0xFF00FF33, 12);
    ImDrawList_AddCircleFilled(dl, (ImVec2){bx, by}, 1.6f, 0xFFFFFFFF, 8);
    ```
  - Player Halo in ImGui (lines 267-272):
    ```c
    if (me) {
      float p_pulse = 5.5f + sinf((float)now * 4.0f) * 1.5f;
      ImDrawList_AddCircleFilled(dl, (ImVec2){p_scr_x, p_scr_y}, p_pulse, 0x5500E5FF, 16);
      ImDrawList_AddCircleFilled(dl, (ImVec2){p_scr_x, p_scr_y}, 3.5f, 0xFF00E5FF, 12);
      ImDrawList_AddCircle(dl, (ImVec2){p_scr_x, p_scr_y}, 4.0f, 0xFFFFFFFF, 12, 1.5f);
    }
    ```

---

### 1.3 Top HUD Badges & Text Overlap Bug
- **Top HUD Implementation (`app/src/game/ui_overlay.c:10-182`)**:
  - `render_stats_hud(env)` draws 4 badges starting at `start_x = 16.0f, start_y = 14.0f`:
    - **Badge 1**: Snake Size (`"Tam: %d"`), gold outline (`0.95f, 0.75f, 0.20f`).
    - **Badge 2**: WiFi Ping (`"%d ms"`), dynamic color (green <85ms, yellow <160ms, red >160ms).
    - **Badge 3**: World Coordinates (`"Coord: (%d, %d)"`), cyan outline.
    - **Badge 4**: Feeder Bots (`"Bots: %d/%d (%.0fu)"` or `"Bots: OFF"`), neon green outline, toggleable via touch click.
- **Root Cause of Text Overlap (Direct Evidence in `v26_play_sec_08.png`)**:
  - In `ui_overlay.c:316-364`, legacy stats text is rendered in the main ImGui window starting at `(4, 4)`:
    - Line 325: `usrs->nickname` ("slither")
    - Line 330: `usrs->ipv4` ("20.2...")
    - Line 347: `"%d ms"` ("203 ms")
    - Line 351: `"%d FPS"` ("61 FPS")
    - Line 361: `"%02d:%02d:%02d"` ("00:00:36")
  - `render_stats_hud` renders on `fg_dl` at `(16, 14)`. The legacy text sits DIRECTLY UNDERNEATH Badge 1 and Badge 2, bleeding through their background rectangles!
- **Minimap Occlusion Bug (Direct Evidence in `v26_play_sec_08.png`)**:
  - Default `minimap_size = 300` px (`user_settings.c:23`). In landscape 1600x720, the minimap spans `X: 1296..1596, Y: 392..692`.
  - In `custom_controls.c:62-73`, the `Turbo` button is placed at `pos_x = 0.88f, pos_y = 0.78f` (`X = 1408, Y = 561`, radius 56px).
  - The `Turbo` button sits directly on top of the minimap center (distance from minimap center is only 42 pixels), obscuring player position, feeder dots, and radar view!

---

### 1.4 Android Build System & Physical Device Verification Setup
- **Android Configuration**:
  - Gradle: 8.5 with AGP 8.2.2.
  - NDK: `26.1.10909125`.
  - SDK: `compileSdk 34`, `targetSdk 34`, `minSdk 24`.
  - ABIs: `arm64-v8a`, `armeabi-v7a`, `x86_64`.
  - Asset sync: `syncAssets` task automatically copies `app/res/` to `app/src/main/assets/app/res/`.
- **Target Physical Device**:
  - IP/Port: `192.168.7.12:33203`.
  - Verified online with `adb devices` -> `192.168.7.12:33203 device`.
  - Model: TECNO KF6p.
  - OS: Android 11 (API 30).
  - Architecture: `arm64-v8a`.
  - Screen resolution: `720x1600` portrait (Physical density: 320 dpi). Active landscape mode: `1600x720`.
  - Package installed: `com.yslither.game`.
- **Screen & Wakefulness Command**:
  - Power state check: `adb -s 192.168.7.12:33203 shell dumpsys power | grep mWakefulness=`
  - Wake command: `adb -s 192.168.7.12:33203 shell "input keyevent KEYCODE_WAKEUP; wm dismiss-keyguard"`
  - Screenshot capture: `adb -s 192.168.7.12:33203 shell "screencap -p /sdcard/snap.png" && adb -s 192.168.7.12:33203 pull /sdcard/snap.png snap.png && adb -s 192.168.7.12:33203 shell "rm /sdcard/snap.png"`
- **Touch Coordinates in Landscape (1600x720)**:
  - Play button: `input tap 800 360`
  - BOTS (Feeder toggle): `input tap 1312 374`
  - BOT (Snake AI toggle): `input tap 1312 274`

---

## 2. Logic Chain

1. **Minimap Boundary Drift Root Cause**:
   - In Slither protocol (`network/callback.c:183, 220`), `grd` is the nominal arena boundary radius (e.g., 21600) and world center is `(grd, grd)`.
   - The actual dynamic boundary ring oscillates at `flux_grd` (starts at `grd * 0.98 = 21168` and is updated by packet `a` and packet `b`/`oef`).
   - The Vulkan shader `mm.slang` draws the arena border ring between `d = SHADOW (0.9)` and `1.0`. The divisor used in the shader is `global.bd_radius`, which `redraw.c:1507` sets to `gdata->data.flux_grd`.
   - In contrast, `ui_overlay.c:196` uses `float arena_r = (gdata->data.grd > 0.0f) ? gdata->data.grd : 21600.0f;` and computes `p_nx = (me->xx - arena_r) / arena_r`.
   - Because `arena_r = grd` instead of `flux_grd`, when an entity is at the arena boundary (`dist == flux_grd`), its normalized distance is `flux_grd / grd = 0.98`. Multiplied by `mm_rad`, it falls short of the shader's boundary ring by 2%. As `flux_grd` pulsates dynamically during gameplay, entity markers drift relative to the rim.
   - **Correction Formula**:
     - Arena center: `cx_world = (gdata->data.grd > 0.0f) ? gdata->data.grd : 21600.0f;`
     - Arena boundary radius: `rad_world = (gdata->data.flux_grd > 0.0f) ? gdata->data.flux_grd : (cx_world * 0.98f);`
     - Normalized coordinates:
       `nx = (x - cx_world) / rad_world;`
       `ny = (y - cx_world) / rad_world;`
     - Screen coordinates:
       `screen_x = mm_cx + nx * mm_rad;`
       `screen_y = mm_cy + ny * mm_rad;`

2. **Double Player Marker Discrepancy**:
   - `mm.slang` renders a pink dot at `global.view` (`view_xx = me->xx + me->fx + fvx`).
   - `ui_overlay.c` renders a cyan halo at `me->xx`.
   - Because `view_xx` leads the snake head with forward velocity offset `fvx, fvy`, the shader dot leads the snake head while the ImGui dot follows the head. As the snake turns or boosts, two dots appear and separate.
   - In the Slither client, the player position is a single distinct marker. `mm.slang` can either be suppressed from drawing the internal pink dot (`ap = 0`), or aligned with the same coordinate source, letting the high-visibility ImGui cyan pulsing halo represent the player exclusively.

3. **Text Overlap & Layout Remediation**:
   - `render_stats_hud` places modern translucent badges at `start_x = 16, start_y = 14`.
   - The legacy desktop stats text block in `ui_overlay.c:316-363` (`nickname`, `ipv4`, `ping`, `fps`, `timer`) is placed at `(4, 4)` and overlaps Badges 1 and 2.
   - Moving the legacy stats block down to `igSetCursorPosY(start_y + card_h + 10.0f)` or eliminating duplicate lines (nickname and server IP are redundant in gameplay; ping is already in Badge 2) immediately eliminates all text clipping and overlap.

4. **Minimap Occlusion Remediation**:
   - Setting mobile `minimap_size` to 220px (`#ifdef __ANDROID__ usrs->minimap_size = 220; #endif`) and/or adjusting `Turbo` button position to `pos_x = 0.90f, pos_y = 0.82f` eliminates overlap between touch controls and the radar minimap.

5. **Feeder Bot Homing Lines & Neon Green Precision**:
   - Neon Green: ImGui little-endian `0xFF00FF33` or `IM_COL32(0, 255, 60, 255)` produces brilliant neon green.
   - Homing Line: Replace static dim line `0x5500FF66` with:
     1. Vibrant guide line: `IM_COL32(0, 255, 80, 160)` with thickness `1.6f`.
     2. Directional animated pulse dot moving along the homing vector `(bot -> player)`:
        `float t = fmodf((float)now * 2.5f + i * 0.4f, 1.0f);`
        `ImVec2 pulse_pt = { bx + (p_scr_x - bx) * t, by + (p_scr_y - by) * t };`
        `ImDrawList_AddCircleFilled(dl, pulse_pt, 2.2f, 0xFFFFFFFF, 8);`
     3. Small directional arrow / chevron pointing at the player snake.

---

## 3. Caveats

1. **Read-Only Scope**: This report is purely analytical. No source code modifications were made.
2. **Shader Recompilation Dependency**: Modifying `mm.slang` requires `slangc`. If `slangc` is not installed on the build machine, shader adjustments should be made either by keeping `mm.slang` as-is and drawing cleanly over it via Dear ImGui, or using SPIR-V disassembly/patching if required.
3. **Screen Orientation Handling**: The target device is natively portrait (720x1600) running in `sensorLandscape` (1600x720). All touch and screen coordinates must remain normalized or scaled to `(1600, 720)`.

---

## 4. Conclusion

1. **R4 Minimap Root Cause**: Coordinate drift is caused by dividing world offsets by static `grd` (21600) instead of dynamic `flux_grd` (21168 and oscillating).
2. **R4 HUD Root Cause**: Layout overlap is caused by legacy stats text rendering at `(4, 4)` directly underneath the top badges rendered at `(16, 14)`.
3. **Minimap Control Occlusion**: Minimap default size (300px) and `Turbo` button placement (`1408, 561`) cause a direct collision that hides the minimap under the player's right thumb button.
4. **Physical Device State**: Device `192.168.7.12:33203` (TECNO KF6p, Android 11, 1600x720 landscape) is online, responsive, and ready for automated testing.
5. **Project Rules Compliance**: Codebase complies 100% with `android-native-vulkan.md` (FIFO present mode, swapchain orientation swap, heap allocation, asset extraction, ImGui touch event forwarding).

---

## 5. Verification Method

### 5.1 Independent Code Verification
Inspect the following exact locations:
- `app/src/game/ui_overlay.c:196-209` (minimap coordinate transform using `arena_r = grd`)
- `app/src/game/ui_overlay.c:316-364` vs `ui_overlay.c:10-182` (legacy stats text colliding with `render_stats_hud`)
- `app/src/game/custom_controls.c:67-68` vs `ui_overlay.c:467-472` (`Turbo` button covering minimap)
- `thermite/src/graphics/tcontext.c:159, 244-248, 296-301` (Vulkan compliance)

### 5.2 Device Verification Commands
Execute against target device `192.168.7.12:33203`:
```bash
# 1. Wake device and verify orientation
adb -s 192.168.7.12:33203 shell "input keyevent KEYCODE_WAKEUP; wm dismiss-keyguard"

# 2. Build and install APK
./gradlew assembleDebug
adb -s 192.168.7.12:33203 install -r app/build/outputs/apk/debug/app-debug.apk

# 3. Launch game and enter match
adb -s 192.168.7.12:33203 shell am start -n com.yslither.game/android.app.NativeActivity
sleep 4
adb -s 192.168.7.12:33203 shell input tap 800 360 # Tap JUGAR

# 4. Activate Feeder Bots & Snake AI
sleep 2
adb -s 192.168.7.12:33203 shell input tap 1312 374 # Toggle BOTS
adb -s 192.168.7.12:33203 shell input tap 1312 274 # Toggle BOT

# 5. Capture gameplay screenshot & verify layout
adb -s 192.168.7.12:33203 shell "screencap -p /sdcard/hud_verify.png"
adb -s 192.168.7.12:33203 pull /sdcard/hud_verify.png .
adb -s 192.168.7.12:33203 shell rm /sdcard/hud_verify.png
```

### 5.3 Invalidation Conditions
- Any coordinate calculation on the minimap that divides by `grd` instead of `flux_grd` invalidates 1:1 arena rim alignment.
- Any text element rendered inside `(start_x..start4_x + card4_w, start_y..start_y + card_h)` invalidates clean top HUD rendering.
- Any touch button placed inside `(minimap_x..minimap_x + minimap_size, minimap_y..minimap_y + minimap_size)` invalidates minimap visibility.
