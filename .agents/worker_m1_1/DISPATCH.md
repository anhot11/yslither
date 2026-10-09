## 2026-09-22T16:48:04Z

You are the Bot AI Overhaul Worker for Milestone 1 (Requirement R1) of the yslither project.
Your Working Directory: /root/.gemini/antigravity-cli/scratch/yslither/.agents/worker_m1_1
Project Directory: /root/.gemini/antigravity-cli/scratch/yslither
Original Request: /root/.gemini/antigravity-cli/scratch/yslither/.agents/ORIGINAL_REQUEST.md
Project Specification: /root/.gemini/antigravity-cli/scratch/yslither/PROJECT.md
Project Rules: /root/.gemini/antigravity-cli/scratch/yslither/.agents/rules/android-native-vulkan.md
Explorer Survey 1 Report: /root/.gemini/antigravity-cli/scratch/yslither/.agents/explorer_survey_1/handoff.md

MANDATORY INTEGRITY WARNING:
DO NOT CHEAT. All implementations must be genuine. DO NOT hardcode test results, create dummy/facade implementations, or circumvent the intended task. A teamwork_preview_auditor will independently verify your work. Integrity violations WILL be detected and your work WILL be rejected.

Mission:
Implement all fixes for Milestone 1 (Requirement R1 - Autonomous Player Bot AI Overhaul).
Files Owned Exclusively:
- app/src/game/sbot.c
- app/src/game/sbot.h
- app/src/game/input.c

Implementation Details (per Explorer Survey 1 recommendations):
1. Eliminate Jitter and 180° Oscillations:
   - In sbot.c, replace discrete 32-ray snapping overwrites with continuous smooth goal navigation.
   - Fix angular turn sign calculation in sbot.c:1171-1178 (rear/flank target pathing) so d_turn near +-PI does not oscillate turn_sign between +1.0 and -1.0.
   - In input.c: smooth quantized angle sang using low-pass filtering / exponential moving average so the snake steers smoothly without jitter.
2. High-Value Food & Corpse Prioritization:
   - Improve compute_food_goal with proactive pathing and multi-factor evaluation (size, distance, density) favoring dense corpse clusters and feeder bot drops over tiny distant orbs.
3. 360-Degree Predictive Obstacle Avoidance:
   - Enforce a strict safe clearance buffer of at least 500 units from enemy snake heads.
   - Repel smoothly from enemy snake bodies and arena rims (flux_grd) using dynamic repulsion vector fields.
4. Feeder Bot Friendliness:
   - Identify allied feeder bots (strncmp(s->nk, "[FEED]", 6) == 0). Do NOT treat them as hostile enemies in get_collision_points or evasion. Allow feeder bots to approach and crash into player snake body segments.
5. Defensive Coiling Overhaul:
   - Fix small snake coiling trap: prevent snakes with length <= 6 from entering blind spin loops.
   - Maintain obstacle sensing during defensive coiling maneuvers.

Verification:
Run ./gradlew assembleDebug to verify compilation passes with zero errors.
Write your completion report to /root/.gemini/antigravity-cli/scratch/yslither/.agents/worker_m1_1/handoff.md and notify parent (conversation ID: b673f1a9-0adf-43e6-bf42-cddae60d4e2d).
