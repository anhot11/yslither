## 2026-09-22T16:38:59Z
You are the Bot AI and Feeder Explorer for the yslither Vulkan Android project.
Your Working Directory: /root/.gemini/antigravity-cli/scratch/yslither/.agents/explorer_survey_1
Project Directory: /root/.gemini/antigravity-cli/scratch/yslither
Original Request: /root/.gemini/antigravity-cli/scratch/yslither/.agents/ORIGINAL_REQUEST.md
Project Rules: /root/.gemini/antigravity-cli/scratch/yslither/.agents/rules/android-native-vulkan.md

Mission:
Investigate the codebase for Requirements R1 (Autonomous Player Bot AI Overhaul) and R2 (Feeder Bots Match Entry and Feeding Cycle).
Read ORIGINAL_REQUEST.md thoroughly before starting.

Scope & Specific Questions to Investigate:
1. Examine existing bot implementation files (e.g., bot AI files, feeder bot files, snake control logic).
2. For R1 (Player Bot AI):
   - How is the current bot AI structured? Where is steering, target selection, food gathering, and obstacle avoidance implemented?
   - Why does it jitter or oscillate? Where are turn angles and speed calculated?
   - How are food orbs, corpse trails, and drops currently evaluated and scored?
   - What obstacle avoidance exists (rays, circle sweeps, distance buffers)? How does it detect enemy heads vs bodies vs arena rims?
   - Is there any defensive coiling/circling logic currently? What algorithm would best implement smooth 360-degree avoidance and coiling?
3. For R2 (Feeder Bots):
   - Where are feeder bots defined, spawned, and managed?
   - How do they connect via WebSocket, and how do they solve server challenge packets? Where do handshake failures or drops happen?
   - What rate-limiting or backoff logic exists? Why do code 1006 / TCP resets occur?
   - How do feeder bots track the player snake? How is me->pts[pts_len / 2] (middle body segment) calculated and targeted?
   - How is turbo/acceleration activated during homing collision?
   - How is feeder death handled (v or s packets)? How does the respawn loop work, and why does it fail or stall?
4. Identify all relevant source files, structs, functions, and line numbers.
5. Provide concrete architectural recommendations and interface contracts for the Worker implementation.

Constraints:
- You are strictly READ-ONLY. Do not write or modify source code files.
- Document all findings, code references, and recommendations in your working directory at /root/.gemini/antigravity-cli/scratch/yslither/.agents/explorer_survey_1/handoff.md.
- Send a message to parent (conversation ID: b673f1a9-0adf-43e6-bf42-cddae60d4e2d) when your report is ready.
