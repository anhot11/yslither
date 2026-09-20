#ifndef TOUCH_INPUT_H
#define TOUCH_INPUT_H

#include <stdbool.h>

typedef enum {
  TOUCH_CONTROL_JOYSTICK = 0,
  TOUCH_CONTROL_POINTER = 1
} touch_control_mode;

typedef struct touch_state {
  touch_control_mode mode;
  bool active;

  // Output aim vector and boost
  float target_x;
  float target_y;
  bool boost;

  // Joystick specific
  bool joy_active;
  int joy_pointer_id;
  float joy_center_x;
  float joy_center_y;
  float joy_curr_x;
  float joy_curr_y;
  float joy_radius;

  // Boost button specific
  bool boost_active;
  int boost_pointer_id;
  float boost_center_x;
  float boost_center_y;
  float boost_radius;
} touch_state;

void touch_input_init(touch_state* ts);
void touch_input_update_layout(touch_state* ts, float screen_w, float screen_h);
void touch_input_down(touch_state* ts, int pointer_id, float x, float y, float screen_w, float screen_h);
void touch_input_move(touch_state* ts, int pointer_id, float x, float y, float screen_w, float screen_h);
void touch_input_up(touch_state* ts, int pointer_id);

#endif
