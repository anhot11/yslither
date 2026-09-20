#ifndef FLIGHT_RECORDER_H
#define FLIGHT_RECORDER_H

#include <thermite.h>
#include <stdbool.h>

#define TELEMETRY_RING_SIZE 300

typedef struct {
  float time_sec;
  float my_x, my_y;
  float my_ang;
  float my_speed;
  int score;
  int kills;
  float goal_x, goal_y;
  int stage;
  bool accel;
  int threat_count;
  float nearest_threat_dist;
  float nearest_threat_x, nearest_threat_y;
} telemetry_frame_t;

void flight_recorder_init(void);
void flight_recorder_record_frame(tenv* env);
void flight_recorder_on_death(tenv* env);
bool flight_recorder_has_death_event(void);
const char* flight_recorder_get_last_death_summary(void);

#endif
