#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "../../app/src/game/sbot_weights.h"

#define MAX_SIM_SNAKES 16
#define MAX_SIM_PTS 400
#define MAX_SIM_FOOD 1024

#define DEATH_CAUSE_NONE 0
#define DEATH_CAUSE_BODY 1
#define DEATH_CAUSE_HEAD 2
#define DEATH_CAUSE_BORDER 3

#define BOT_TYPE_SUBJECT 0
#define BOT_TYPE_AGGRESSIVE 1
#define BOT_TYPE_WANDERER 2
#define BOT_TYPE_FARMER 3

typedef struct {
  bool alive;
  int id;
  int bot_type;
  float x, y;
  float ang;
  float target_ang;
  float sp;
  bool boosting;
  float mass;
  float max_mass;
  int len;
  float pts_x[MAX_SIM_PTS];
  float pts_y[MAX_SIM_PTS];
  int kills;
  float survival_time;
  int death_cause;
  sbot_weights_t weights;
  float smooth_ang;
} sim_snake_t;

typedef struct {
  float x, y;
  float sz;
  bool active;
} sim_food_t;

typedef struct {
  uint64_t seed;
  float survival_time;
  float max_mass;
  int final_len;
  int kills;
  int death_cause;
} sim_match_result_t;

typedef struct {
  uint64_t seed;
  float max_duration_sec;
  int num_rivals;
  float arena_radius;
  const char* jsonl_log_path;
} sim_match_config_t;

typedef struct {
  sim_match_config_t cfg;
  uint64_t rng_state;
  float sim_time;
  sim_snake_t snakes[MAX_SIM_SNAKES];
  int num_snakes;
  sim_food_t foods[MAX_SIM_FOOD];
  int num_foods;
  float arena_radius;
} sim_world_t;

// PRNG (xorshift64star)
static inline uint64_t sim_rng_next(uint64_t* state) {
  uint64_t x = *state;
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  *state = x;
  return x * 0x2545F4914F6CDD1DULL;
}

static inline float sim_rng_float(uint64_t* state, float min_v, float max_v) {
  uint64_t r = sim_rng_next(state) >> 32;
  float f = (float)r / 4294967295.0f;
  return min_v + f * (max_v - min_v);
}

// Simulator API
void sim_init(sim_world_t* w, const sim_match_config_t* cfg, const sbot_weights_t* subject_weights);
sim_match_result_t sim_run_match(sim_world_t* w);

#endif // SIMULATOR_H
