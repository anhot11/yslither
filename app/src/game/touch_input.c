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
}

bool touch_input_is_boosting(void) {
  return s_current_touch_state ? s_current_touch_state->boost : false;
}

void touch_input_update_layout(touch_state* ts, float screen_w, float screen_h) {
  // Boost button positioned in the bottom right corner
  ts->boost_center_x = screen_w - 140.0f;
  ts->boost_center_y = screen_h - 140.0f;
  ts->boost_radius = 90.0f;
}

void touch_input_down(touch_state* ts, int pointer_id, float x, float y, float screen_w, float screen_h) {
  touch_input_update_layout(ts, screen_w, screen_h);

  // Check if touching boost button in the right area
  float bdx = x - ts->boost_center_x;
  float bdy = y - ts->boost_center_y;
  if (bdx * bdx + bdy * bdy <= ts->boost_radius * ts->boost_radius * 1.5f) {
    ts->boost = true;
    ts->boost_active = true;
    ts->boost_pointer_id = pointer_id;
    return;
  }

  if (ts->mode == TOUCH_CONTROL_JOYSTICK) {
    // Left half of screen initiates joystick
    if (x < screen_w * 0.65f && !ts->joy_active) {
      ts->joy_active = true;
      ts->joy_pointer_id = pointer_id;
      ts->joy_center_x = x;
      ts->joy_center_y = y;
      ts->joy_curr_x = x;
      ts->joy_curr_y = y;
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
    ts->boost = (bdx * bdx + bdy * bdy <= (ts->boost_radius * 1.8f) * (ts->boost_radius * 1.8f));
    return;
  }

  if (ts->mode == TOUCH_CONTROL_JOYSTICK) {
    if (pointer_id == ts->joy_pointer_id && ts->joy_active) {
      ts->joy_curr_x = x;
      ts->joy_curr_y = y;

      float dx = x - ts->joy_center_x;
      float dy = y - ts->joy_center_y;
      float d = sqrtf(dx * dx + dy * dy);

      if (d > 10.0f) {
        ts->target_x = dx;
        ts->target_y = dy;
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
