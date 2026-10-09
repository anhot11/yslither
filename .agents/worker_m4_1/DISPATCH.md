## 2026-09-22T16:48:04Z

Minimap & HUD Worker for Milestone 4 (Requirement R4) of the yslither project.
Your Working Directory: /root/.gemini/antigravity-cli/scratch/yslither/.agents/worker_m4_1
Project Directory: /root/.gemini/antigravity-cli/scratch/yslither
Original Request: /root/.gemini/antigravity-cli/scratch/yslither/.agents/ORIGINAL_REQUEST.md
Project Specification: /root/.gemini/antigravity-cli/scratch/yslither/PROJECT.md
Project Rules: /root/.gemini/antigravity-cli/scratch/yslither/.agents/rules/android-native-vulkan.md
Explorer Survey 3 Report: /root/.gemini/antigravity-cli/scratch/yslither/.agents/explorer_survey_3/handoff.md

Mission:
Implement all fixes for Milestone 4 (Requirement R4 - Minimap and HUD Rendering Precision).
Files Owned Exclusively:
- app/src/game/ui_overlay.c
- app/src/game/custom_controls.c
- app/src/rendering/mm_renderer.c

Implementation Details (per Explorer Survey 3 recommendations):
1. 1:1 Circular Minimap Coordinate Mapping:
   In ui_overlay.c:196-210, fix coordinate normalization: divide by dynamic boundary flux_grd (gdata->data.flux_grd > 0 ? gdata->data.flux_grd : grd * 0.98f) instead of static grd, aligning player and entity markers with the arena boundary ring without drift.
2. Single Distinct Player Marker:
   Ensure the player position is represented by a single distinct marker (cyan pulsing halo in ImGui) without double-dot divergence.
3. Vibrant Neon Green Feeder Bots & Directional Homing Lines:
   - Render feeder bots in brilliant neon green (IM_COL32(0, 255, 60, 255)).
   - Add vibrant directional homing guide lines with animated pulse dots moving along the vector from feeder to player snake, showing homing trajectory clearly.
4. Top HUD Layout & Overlap Remediation:
   - In ui_overlay.c:316-364, remove or relocate the legacy desktop text block rendered at (4, 4) that bleeds directly underneath Badges 1 and 2 (rendered at 16, 14).
   - Ensure the 4 badges (Tam/Size, Ping, Coordinates, Feeder count/distance) render cleanly without text clipping or overlapping.
5. Minimap Touch Occlusion Remediation:
   - On Android, ensure minimap_size is sized appropriately (e.g. 220px).
   - In custom_controls.c, position touch controls (e.g. Turbo button) so they do not cover or obscure the circular minimap.

Verification:
Run ./gradlew assembleDebug to verify compilation passes with zero errors.
Write your completion report to /root/.gemini/antigravity-cli/scratch/yslither/.agents/worker_m4_1/handoff.md and notify parent (conversation ID: b673f1a9-0adf-43e6-bf42-cddae60d4e2d).
