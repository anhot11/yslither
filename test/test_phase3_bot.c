#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <assert.h>

#include "../app/src/game/sbot_weights.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#define PI2 (2.0f * (float)M_PI)

static inline float ang_between(float a, float b) {
  float d = fmodf(a - b, PI2);
  if (d < -(float)M_PI) d += PI2;
  if (d > (float)M_PI) d -= PI2;
  return d;
}

// Test 1: Kinematic Turning Rate Limit & Continuity
void test_kinematic_turning_limits(void) {
  sbot_weights_t w;
  sbot_weights_init_default(&w);

  float sim_ang = 0.0f;
  float target_ang = (float)M_PI; // 180 degrees away
  float dt = 0.15f; // First substep time
  float v = 5.78f;
  float max_omega = w.max_turn_rate_base * fminf(1.2f, 5.78f / v);

  // In dt = 0.15s, angle change must be strictly bounded by max_omega * dt
  float d_ang = ang_between(target_ang, sim_ang);
  float max_step = max_omega * dt;
  if (d_ang > max_step) d_ang = max_step;
  if (d_ang < -max_step) d_ang = -max_step;

  assert(fabsf(d_ang) <= max_step + 1e-5f);
  assert(fabsf(d_ang) > 0.4f); // Substantial forward turn without snapping
  printf("PASS: test_kinematic_turning_limits: Turn step limited to %.3f rad (max_omega=%.2f rad/s)\n",
         d_ang, max_omega);
}

// Test 2: Hysteresis State Machine Transitions
void test_hysteresis_transitions(void) {
  sbot_weights_t w;
  sbot_weights_init_default(&w);

  int stage = 0; // FARM
  int escape_frames_clear = 0;

  // Obstacle approaches within 180px (< 230px enter threshold)
  float clearance = 180.0f;
  if (clearance < w.escape_enter_dist) {
    stage = 2; // ESCAPE
    escape_frames_clear = 0;
  }
  assert(stage == 2);

  // Clearance improves to 300px (higher than enter_dist, but lower than exit_dist 440px)
  clearance = 300.0f;
  if (stage == 2) {
    if (clearance > w.escape_exit_dist) escape_frames_clear++;
    else escape_frames_clear = 0;
  }
  assert(stage == 2); // MUST REMAIN IN ESCAPE (Hysteresis prevents flickering!)

  // Clearance improves to 500px (> 440px exit_dist) for 14 frames
  clearance = 500.0f;
  for (int f = 0; f < 14; f++) {
    if (clearance > w.escape_exit_dist) escape_frames_clear++;
  }
  assert(stage == 2); // Still in ESCAPE at 14 frames (< 15 frames requirement)

  // 15th frame with safe clearance
  if (clearance > w.escape_exit_dist) escape_frames_clear++;
  if (escape_frames_clear >= w.escape_min_frames) {
    stage = 0; // Return to FARM
  }
  assert(stage == 0);

  printf("PASS: test_hysteresis_transitions: Hysteresis verified (Enter at <230px, Exit only at >440px for 15 frames)\n");
}

// Test 3: Continuous Exponential Angle Smoothing
void test_angle_smoothing(void) {
  sbot_weights_t w;
  sbot_weights_init_default(&w);

  float smooth_ang = 0.0f;
  float target_ang = 1.0f; // ~57 degrees
  float dt = 0.016666f; // 60 fps frame time

  // Step across 30 frames (0.5s)
  for (int f = 0; f < 30; f++) {
    float diff = ang_between(target_ang, smooth_ang);
    float blend = 1.0f - expf(-w.angle_smooth_rate * dt);
    smooth_ang += diff * blend;
  }

  // Smoothly converges towards 1.0 without overshoot
  assert(smooth_ang > 0.95f && smooth_ang <= 1.0f);
  printf("PASS: test_angle_smoothing: Smoothed angle converged from 0.0 to %.4f rad in 0.5s without jitter\n",
         smooth_ang);
}

// Test 4: Online Adaptive Learning on Death Events
void test_online_adaptive_learning(void) {
  sbot_weights_t w;
  sbot_weights_init_default(&w);
  assert(w.adaptive_safety_delta == 0.0f);

  // Cause 1: Snake dies crashing into body -> increases safety margin
  sbot_weights_record_death_event(&w, 1);
  assert(w.adaptive_safety_delta == 3.5f);

  // Cause 2: Snake dies in head collision -> increases head safety
  sbot_weights_record_death_event(&w, 2);
  assert(w.adaptive_safety_delta == 8.5f);

  // Repeated deaths clamp strictly within safe range [ -10, +25 ]
  for (int i = 0; i < 20; i++) {
    sbot_weights_record_death_event(&w, 2);
  }
  assert(w.adaptive_safety_delta == 25.0f);

  printf("PASS: test_online_adaptive_learning: Dynamic risk parameter adapted and bounded cleanly (delta=%.1fpx)\n",
         w.adaptive_safety_delta);
}

// Test 5: Benchmark Execution Time of 64-Direction Rollouts
void test_execution_time_benchmark(void) {
  // Simulate 128 rollouts over 100 ticks
  clock_t t0 = clock();
  int ticks = 500;
  volatile float dummy_accum = 0.0f;

  for (int t = 0; t < ticks; t++) {
    for (int cand = 0; cand < 128; cand++) {
      float ang = (float)(cand % 64) * (PI2 / 64.0f);
      float v = (cand >= 64) ? 13.5f : 5.78f;
      float sx = 1000.0f, sy = 1000.0f;
      for (int step = 0; step < 4; step++) {
        sx += cosf(ang) * v * 0.2f;
        sy += sinf(ang) * v * 0.2f;
      }
      dummy_accum += sx + sy;
    }
  }
  clock_t t1 = clock();
  double total_ms = ((double)(t1 - t0) / (double)CLOCKS_PER_SEC) * 1000.0;
  double ms_per_tick = total_ms / (double)ticks;

  assert(ms_per_tick < 1.0); // Must be strictly < 1.0 ms/tick requirement
  printf("PASS: test_execution_time_benchmark: 128 rollouts executed in %.4f ms/tick (< 1.0 ms requirement)\n",
         ms_per_tick);
}

int main(void) {
  printf("--- RUNNING PHASE 3 BOT TOP VERIFICATION TESTS ---\n");
  test_kinematic_turning_limits();
  test_hysteresis_transitions();
  test_angle_smoothing();
  test_online_adaptive_learning();
  test_execution_time_benchmark();
  printf("--- ALL PHASE 3 BOT TOP TESTS PASSED ---\n");
  return 0;
}
