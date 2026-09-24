#include "touch_input.h"

#include <math.h>
#include <string.h>

static touch_state* s_current_touch_state = NULL;

void touch_input_init(touch_state* ts) {
  s_current_touch_state = ts;
  memset(ts, 0, sizeof(touch_state));
  ts->mode = TOUCH_CONTROL_JOYSTICK;
  ts->joy_pointer_id = -1;
  ts->boost_pointer_id = -1;
  ts->joy_radius = 120.0f;
  ts->boost_radius = 90.0f;
  ts->deadzone = 12.0f;
  ts->sensitivity = 1.0f;
  ts->curve_exponent = 1.4f;
  ts->left_handed = false;
  ts->haptic_requested = false;
}

bool touch_input_is_boosting(void) {
  return s_current_touch_state ? s_current_touch_state->boost : false;
}

void touch_input_update_layout(touch_state* ts, float screen_w, float screen_h) {
  if (ts->left_handed) {
    // Left-handed mode: boost button on the bottom-left corner
    ts->boost_center_x = 140.0f;
    ts->boost_center_y = screen_h - 140.0f;
  } else {
    // Standard mode: boost button positioned in the bottom-right corner
    ts->boost_center_x = screen_w - 140.0f;
    ts->boost_center_y = screen_h - 140.0f;
  }
  ts->boost_radius = 90.0f;
}

void touch_input_down(touch_state* ts, int pointer_id, float x, float y, float screen_w, float screen_h) {
  touch_input_update_layout(ts, screen_w, screen_h);

  // Check if touching boost button in the designated corner area
  float bdx = x - ts->boost_center_x;
  float bdy = y - ts->boost_center_y;
  if (bdx * bdx + bdy * bdy <= ts->boost_radius * ts->boost_radius * 1.5f) {
    if (!ts->boost) ts->haptic_requested = true;
    ts->boost = true;
    ts->boost_active = true;
    ts->boost_pointer_id = pointer_id;
    return;
  }

  if (ts->mode == TOUCH_CONTROL_JOYSTICK) {
    bool side_ok = ts->left_handed ? (x >= screen_w * 0.40f) : (x <= screen_w * 0.60f);
    if (!ts->joy_active && side_ok) {
      ts->joy_active = true;
      ts->joy_pointer_id = pointer_id;
      ts->joy_center_x = x;
      ts->joy_center_y = y;
      ts->joy_curr_x = x;
      ts->joy_curr_y = y;
      float cx = screen_w * 0.5f;
      float cy = screen_h * 0.5f;
      ts->target_x = x - cx;
      ts->target_y = y - cy;
      ts->active = true;
    }
  } else {
    // Direct pointer mode: snake steers towards touch point relative to center
    ts->active = true;
    ts->target_x = x - (screen_w / 2.0f);
    ts->target_y = y - (screen_h / 2.0f);
  }
}

void touch_input_move(touch_state* ts, int pointer_id, float x, float y, float screen_w, float screen_h) {
  if (pointer_id == ts->boost_pointer_id) {
    float bdx = x - ts->boost_center_x;
    float bdy = y - ts->boost_center_y;
    bool now_boost = (bdx * bdx + bdy * bdy <= (ts->boost_radius * 1.8f) * (ts->boost_radius * 1.8f));
    if (now_boost && !ts->boost) ts->haptic_requested = true;
    ts->boost = now_boost;
    return;
  }

  if (ts->mode == TOUCH_CONTROL_JOYSTICK) {
    if (pointer_id == ts->joy_pointer_id && ts->joy_active) {
      ts->joy_curr_x = x;
      ts->joy_curr_y = y;

      float dx = x - ts->joy_center_x;
      float dy = y - ts->joy_center_y;
      float d = sqrtf(dx * dx + dy * dy);
      float dz = fmaxf(4.0f, ts->deadzone);

      if (d > dz) {
        float effective_d = (d - dz) / fmaxf(1.0f, (ts->joy_radius - dz));
        if (effective_d > 1.0f) effective_d = 1.0f;
        float curved_d = powf(effective_d, ts->curve_exponent) * ts->sensitivity;
        ts->target_x = (dx / d) * curved_d * 100.0f;
        ts->target_y = (dy / d) * curved_d * 100.0f;
        ts->active = true;
      }
    }
  } else {
    // Pointer mode tracking
    if (pointer_id != ts->boost_pointer_id) {
      ts->active = true;
      ts->target_x = x - (screen_w / 2.0f);
      ts->target_y = y - (screen_h / 2.0f);
    }
  }
}

void touch_input_up(touch_state* ts, int pointer_id) {
  if (pointer_id == ts->boost_pointer_id) {
    ts->boost = false;
    ts->boost_active = false;
    ts->boost_pointer_id = -1;
  }

  if (pointer_id == ts->joy_pointer_id) {
    ts->joy_active = false;
    ts->joy_pointer_id = -1;
  }
}
