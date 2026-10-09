# BRIEFING — 2026-09-22T16:46:15Z

## Mission
Investigate codebase for Requirement R4 (Minimap and HUD Rendering Precision) and Physical Device Verification (Android build, install, ADB device at 192.168.7.12:33203).

## 🔒 My Identity
- Archetype: explorer
- Roles: investigation, synthesis, physical-device-verification, renderer-inspection
- Working directory: /root/.gemini/antigravity-cli/scratch/yslither/.agents/explorer_survey_3
- Original parent: b673f1a9-0adf-43e6-bf42-cddae60d4e2d
- Milestone: exploration_survey

## 🔒 Key Constraints
- Read-only investigation — do NOT modify project source code
- Document all findings in handoff.md in working directory
- Abide by rules in .agents/rules/android-native-vulkan.md

## Current Parent
- Conversation ID: b673f1a9-0adf-43e6-bf42-cddae60d4e2d
- Updated: 2026-09-22T16:46:15Z

## Investigation State
- **Explored paths**:
  - `app/src/rendering/` (`renderer.c`, `mm_renderer.c`, `mm_renderer.h`)
  - `app/res/shaders/src/` (`mm.slang`, `modules/common.slang`)
  - `app/src/game/` (`ui_overlay.c`, `redraw.c`, `feeder.c`, `custom_controls.c`, `loop.c`)
  - `app/src/android_main.c`, `app/src/imgui_setup.c`
  - `thermite/src/graphics/` (`tcontext.c`, `tdbuffer.c`)
  - Build files: `CMakeLists.txt`, `app/build.gradle`, `build.gradle`, `AndroidManifest.xml`
  - Device probe: `adb -s 192.168.7.12:33203` (TECNO KF6p, Android 11, 720x1600 / 1600x720)
  - Visual artifacts: `v26_play_sec_08.png`, `probe_screen.png`
- **Key findings**:
  1. Coordinate drift caused by `ui_overlay.c:196` dividing by static `grd` (21600) instead of dynamic `flux_grd` (which matches shader boundary ring).
  2. Double marker bug: `mm.slang` draws pink player dot at `view_xx` (with camera lead offset), while `ui_overlay.c` draws cyan halo at `me->xx`.
  3. Text overlap: Legacy debug text at `(4, 4)` in `ui_overlay.c:316-364` bleeds through Badges 1 and 2 at `(16, 14)`.
  4. Minimap occlusion: Default `minimap_size` is 300px; `Turbo` touch button (`pos_x 0.88, pos_y 0.78`) renders directly in the center of the minimap.
  5. Directional homing lines: Currently faint line (`0x5500FF66`, 1.2px) without directional animation/arrowheads; feeder bots can be duplicated if drawn by both `feeder_get_bots_pos` and snake loop.
  6. Device verified online and responsive (`192.168.7.12:33203`); screencap and logcat capture verified. Full compliance with `android-native-vulkan.md` confirmed.
- **Unexplored areas**: None within survey scope.

## Key Decisions Made
- Confirmed full evidence chain for coordinate drift, text overlap, and minimap layout collisions.
- Designed concrete architectural fixes for R4 and verification workflow.

## Artifact Index
- /root/.gemini/antigravity-cli/scratch/yslither/.agents/explorer_survey_3/DISPATCH.md — Initial dispatch
- /root/.gemini/antigravity-cli/scratch/yslither/.agents/explorer_survey_3/BRIEFING.md — Situational awareness
- /root/.gemini/antigravity-cli/scratch/yslither/.agents/explorer_survey_3/progress.md — Liveness heartbeat
- /root/.gemini/antigravity-cli/scratch/yslither/.agents/explorer_survey_3/handoff.md — Final investigation report
