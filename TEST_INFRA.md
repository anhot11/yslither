# E2E Test Infra: yslither Vulkan Android Game Client

## Test Philosophy
- Opaque-box, requirement-driven. Derived strictly from ORIGINAL_REQUEST.md and user-facing specifications.
- Methodology: Systematic 4-tier testing (Category-Partition, Boundary Value Analysis, Pairwise Combinations, Real-World Application Workloads).

## Feature Inventory & Test Coverage
| # | Feature | Requirement Source | Tier 1 | Tier 2 | Tier 3 | Tier 4 |
|---|---------|-------------------|:------:|:------:|:------:|:------:|
| 1 | F1: Autonomous Player Bot AI | ORIGINAL_REQUEST §R1 | 5 | 5 | ✓ | ✓ |
| 2 | F2: Feeder Swarm Match & Feeding | ORIGINAL_REQUEST §R2 | 5 | 5 | ✓ | ✓ |
| 3 | F3: Server Connection & Stability | ORIGINAL_REQUEST §R3 | 5 | 5 | ✓ | ✓ |
| 4 | F4: Minimap & HUD Rendering | ORIGINAL_REQUEST §R4 | 5 | 5 | ✓ | ✓ |
| 5 | F5: Physical Device Verification | ORIGINAL_REQUEST §Acceptance Criteria | 5 | 5 | ✓ | ✓ |

## Test Architecture
- **Target Device**: Android 11 physical device at `192.168.7.12:33203` (TECNO KF6p, 1600x720 landscape).
- **Execution Harness**: Python automated E2E test harness connecting via ADB, driving inputs, capturing logcat traces, evaluating game state telemetry, and pulling gameplay screenshots.
- **Pass/Fail Semantics**: Exit code 0, all assertions verified against live telemetry and logcat tags (`yslither`, `yslither_net`, `feeder_bot`, `yslither_loop`).

## Real-World Application Scenarios (Tier 4)
| # | Scenario | Features Exercised | Complexity | Target Criterion |
|---|----------|--------------------|------------|------------------|
| 1 | Tap 'JUGAR' seamless connection | F3, F4 | Medium | Instant match entry, no retry hangs |
| 2 | 60s Autonomous Bot Survival | F1, F3, F4 | High | Player snake survives ≥60s with bot steering, Tam increases from 10 |
| 3 | Concurrent Feeder Homing & Feeding | F1, F2, F4 | High | ≥2 feeders spawn, home into body, die on crash, player feeds and grows |
| 4 | Obstacle Evasion & Safe Clearance | F1 | High | Evasion maintains ≥500u buffer from enemy heads |
| 5 | Feeder Respawn Loop & 0 Rate-Limits | F2, F3 | High | Feeders respawn continuously after death with zero code 1006 drops |

## Coverage Thresholds
- Tier 1: ≥5 test cases per feature area (happy path)
- Tier 2: ≥5 test cases per feature area (edge/boundary: disconnects, rim boundary, small snake coiling, server timeouts)
- Tier 3: Pairwise interactions (Bot AI + Feeders, Network failover + Feeders, Minimap + HUD)
- Tier 4: Realistic long-running device gameplay runs on `192.168.7.12:33203`
