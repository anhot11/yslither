#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>

// Mock structures to test Phase 2 math & ergonomics in isolation
typedef struct {
  float deadzone;
  float sensitivity;
  float curve_exponent;
  float joy_radius;
  float target_x;
  float target_y;
  bool active;
  bool left_handed;
  float boost_center_x;
  float boost_center_y;
  float boost_radius;
} test_touch_state;

void test_touch_update_layout(test_touch_state* ts, float screen_w, float screen_h) {
  if (ts->left_handed) {
    ts->boost_center_x = 140.0f;
    ts->boost_center_y = screen_h - 140.0f;
  } else {
    ts->boost_center_x = screen_w - 140.0f;
    ts->boost_center_y = screen_h - 140.0f;
  }
  ts->boost_radius = 90.0f;
}

void test_touch_eval(test_touch_state* ts, float dx, float dy) {
  float d = sqrtf(dx * dx + dy * dy);
  float dz = fmaxf(4.0f, ts->deadzone);
  if (d > dz) {
    float effective_d = (d - dz) / fmaxf(1.0f, (ts->joy_radius - dz));
    if (effective_d > 1.0f) effective_d = 1.0f;
    float curved_d = powf(effective_d, ts->curve_exponent) * ts->sensitivity;
    ts->target_x = (dx / d) * curved_d * 100.0f;
    ts->target_y = (dy / d) * curved_d * 100.0f;
    ts->active = true;
  } else {
    ts->target_x = 0.0f;
    ts->target_y = 0.0f;
    ts->active = false;
  }
}

// Test 1: Touch Deadzone and Power Response Curve
void test_touch_curve_and_deadzone(void) {
  test_touch_state ts = {
    .deadzone = 12.0f,
    .sensitivity = 1.0f,
    .curve_exponent = 1.4f,
    .joy_radius = 120.0f,
    .target_x = 0,
    .target_y = 0,
    .active = false
  };

  // Inside deadzone (e.g. 8px offset) -> must remain inactive and zero
  test_touch_eval(&ts, 6.0f, 6.0f); // d = 8.48px < 12px
  assert(!ts.active);
  assert(ts.target_x == 0.0f && ts.target_y == 0.0f);

  // Just outside deadzone (e.g. 15px offset) -> small response, power curve attenuates micro-jitters
  test_touch_eval(&ts, 15.0f, 0.0f);
  assert(ts.active);
  assert(ts.target_x > 0.0f && ts.target_x < 2.0f); // (3/108)^1.4 * 100 ~ 0.65

  // Full deflection (120px offset) -> reaches full 100.0f output
  test_touch_eval(&ts, 120.0f, 0.0f);
  assert(fabsf(ts.target_x - 100.0f) < 0.1f);

  printf("PASS: test_touch_curve_and_deadzone: deadzone rejected (<12px), power curve smooth response verified.\n");
}

// Test 2: Left-Handed Touch Layout Inversion
void test_left_handed_layout(void) {
  test_touch_state ts_right = { .left_handed = false };
  test_touch_state ts_left = { .left_handed = true };
  float screen_w = 2400.0f;
  float screen_h = 1080.0f;

  test_touch_update_layout(&ts_right, screen_w, screen_h);
  test_touch_update_layout(&ts_left, screen_w, screen_h);

  // Standard: boost on right
  assert(ts_right.boost_center_x == screen_w - 140.0f);
  // Left-handed: boost on left
  assert(ts_left.boost_center_x == 140.0f);

  printf("PASS: test_left_handed_layout: standard=%.0fpx, left-handed=%.0fpx verified.\n",
         ts_right.boost_center_x, ts_left.boost_center_x);
}

// Test 3: Camera Tracking & Smooth Zoom Critical Damping
void test_camera_and_zoom_dampening(void) {
  float view_x = 1000.0f;
  float target_x = 1200.0f; // 200px step
  float dt = 0.016666f; // 60 fps

  for (int f = 0; f < 60; f++) {
    float cam_rate = 1.0f - expf(-18.0f * dt);
    view_x += (target_x - view_x) * cam_rate;
  }

  // After 60 frames (1s), view_x should have smoothly converged to target within 0.01px
  assert(fabsf(target_x - view_x) < 0.01f);

  // Zoom convergence test
  float gsc = 1.0f;
  float target_zoom = 0.65f;
  for (int f = 0; f < 60; f++) {
    float zoom_blend = 1.0f - expf(-10.0f * dt);
    gsc += (target_zoom - gsc) * zoom_blend;
  }
  assert(fabsf(target_zoom - gsc) < 0.01f);

  printf("PASS: test_camera_and_zoom_dampening: camera diff=%.5fpx, zoom diff=%.5f after 1.0s.\n",
         fabsf(target_x - view_x), fabsf(target_zoom - gsc));
}

int main(void) {
  printf("--- RUNNING PHASE 2 QUALITY & POLISH VERIFICATION TESTS ---\n");
  test_touch_curve_and_deadzone();
  test_left_handed_layout();
  test_camera_and_zoom_dampening();
  printf("--- ALL PHASE 2 POLISH UNIT TESTS PASSED ---\n");
  return 0;
}
