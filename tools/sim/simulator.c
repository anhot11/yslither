#include "simulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#define PI2 (2.0f * (float)M_PI)

static inline float sim_dist2(float ax, float ay, float bx, float by) {
  float dx = ax - bx, dy = ay - by;
  return dx * dx + dy * dy;
}

static inline float sim_ang_diff(float a, float b) {
  float d = fmodf(a - b, PI2);
  if (d < -(float)M_PI) d += PI2;
  if (d > (float)M_PI) d -= PI2;
  return d;
}

static void sim_spawn_snake(sim_world_t* w, int idx, int type, const sbot_weights_t* weights) {
  sim_snake_t* s = &w->snakes[idx];
  memset(s, 0, sizeof(sim_snake_t));
  s->alive = true;
  s->id = idx;
  s->bot_type = type;

  if (idx == 0) {
    // Subject bot in center
    s->x = 0.0f;
    s->y = 0.0f;
    s->ang = sim_rng_float(&w->rng_state, 0.0f, PI2);
  } else {
    float r = sim_rng_float(&w->rng_state, 400.0f, w->arena_radius * 0.70f);
    float a = sim_rng_float(&w->rng_state, 0.0f, PI2);
    s->x = cosf(a) * r;
    s->y = sinf(a) * r;
    s->ang = sim_rng_float(&w->rng_state, 0.0f, PI2);
  }

  s->target_ang = s->ang;
  s->smooth_ang = s->ang;
  s->sp = 5.78f;
  s->boosting = false;
  s->mass = (type == BOT_TYPE_SUBJECT) ? 22.0f : sim_rng_float(&w->rng_state, 18.0f, 65.0f);
  s->max_mass = s->mass;
  s->len = 15 + (int)(s->mass * 0.6f);
  if (s->len > MAX_SIM_PTS) s->len = MAX_SIM_PTS;

  for (int p = 0; p < s->len; p++) {
    s->pts_x[p] = s->x - cosf(s->ang) * (p * 8.0f);
    s->pts_y[p] = s->y - sinf(s->ang) * (p * 8.0f);
  }

  if (weights) {
    s->weights = *weights;
  } else {
    sbot_weights_init_default(&s->weights);
  }
}

void sim_init(sim_world_t* w, const sim_match_config_t* cfg, const sbot_weights_t* subject_weights) {
  memset(w, 0, sizeof(sim_world_t));
  w->cfg = *cfg;
  w->rng_state = cfg->seed ? cfg->seed : 123456789ULL;
  w->arena_radius = (cfg->arena_radius > 500.0f) ? cfg->arena_radius : 3200.0f;
  w->sim_time = 0.0f;

  w->num_snakes = 1 + cfg->num_rivals;
  if (w->num_snakes > MAX_SIM_SNAKES) w->num_snakes = MAX_SIM_SNAKES;

  // Spawn subject snake
  sim_spawn_snake(w, 0, BOT_TYPE_SUBJECT, subject_weights);

  // Spawn rivals with varied behavior archetypes
  for (int i = 1; i < w->num_snakes; i++) {
    int btype = (i % 3) + 1; // 1 = Aggressive, 2 = Wanderer, 3 = Farmer
    sim_spawn_snake(w, i, btype, NULL);
  }

  // Populate arena food
  w->num_foods = 320;
  for (int i = 0; i < w->num_foods; i++) {
    float r = sqrtf(sim_rng_float(&w->rng_state, 0.0f, 1.0f)) * (w->arena_radius * 0.90f);
    float a = sim_rng_float(&w->rng_state, 0.0f, PI2);
    w->foods[i] = (sim_food_t){
      .x = cosf(a) * r,
      .y = sinf(a) * r,
      .sz = sim_rng_float(&w->rng_state, 1.0f, 3.5f),
      .active = true
    };
  }
}

// Decision step for Subject Bot using 64-direction kinematic rollouts
static void sim_step_subject_ai(sim_world_t* w, sim_snake_t* s, float dt) {
  sbot_weights_t* weights = &s->weights;

  float best_score = -1e9f;
  float best_ang = s->ang;
  bool best_boost = false;

  float s_rad = 12.0f + s->mass * 0.06f;

  // 64 directions evaluation
  for (int cand = 0; cand < 128; cand++) {
    int dir_idx = cand % 64;
    bool boost = (cand >= 64);
    float target_ang = (float)dir_idx * (PI2 / 64.0f);
    float v = boost ? 13.5f : 5.78f;
    float max_omega = weights->max_turn_rate_base * fminf(1.2f, 5.78f / v);

    float sim_x = s->x;
    float sim_y = s->y;
    float sim_ang = s->ang;
    bool collides = false;

    // 4 simulation substeps
    for (int step = 0; step < 4; step++) {
      float sub_dt = 0.20f;
      float dang = sim_ang_diff(target_ang, sim_ang);
      float step_turn = max_omega * sub_dt;
      if (dang > step_turn) dang = step_turn;
      if (dang < -step_turn) dang = -step_turn;
      sim_ang += dang;
      sim_x += cosf(sim_ang) * (v * sub_dt);
      sim_y += sinf(sim_ang) * (v * sub_dt);

      // Arena border collision
      if (sqrtf(sim_x * sim_x + sim_y * sim_y) + s_rad + weights->safety_margin_base > w->arena_radius) {
        collides = true;
        break;
      }

      // Check collision with rival snake bodies
      for (int i = 1; i < w->num_snakes; i++) {
        sim_snake_t* rival = &w->snakes[i];
        if (!rival->alive) continue;

        // Rival head
        float hd2 = sim_dist2(sim_x, sim_y, rival->x, rival->y);
        float r_head = s_rad + 14.0f + weights->safety_margin_base;
        if (hd2 < r_head * r_head && rival->mass >= s->mass * 0.95f) {
          collides = true;
          break;
        }

        // Rival body
        for (int p = 0; p < rival->len; p += 2) {
          float pd2 = sim_dist2(sim_x, sim_y, rival->pts_x[p], rival->pts_y[p]);
          float req_r = s_rad + 10.0f + weights->safety_margin_base + weights->adaptive_safety_delta;
          if (pd2 < req_r * req_r) {
            collides = true;
            break;
          }
        }
        if (collides) break;
      }
      if (collides) break;
    }

    if (collides) continue;

    // Raycast clearance
    float ray_clear = 800.0f;
    for (int i = 1; i < w->num_snakes; i++) {
      sim_snake_t* rival = &w->snakes[i];
      if (!rival->alive) continue;
      for (int p = 0; p < rival->len; p += 4) {
        float vx = rival->pts_x[p] - sim_x;
        float vy = rival->pts_y[p] - sim_y;
        float proj = vx * cosf(sim_ang) + vy * sinf(sim_ang);
        if (proj > 0.0f && proj < ray_clear) {
          float perp2 = (vx * vx + vy * vy) - (proj * proj);
          if (perp2 < 45.0f * 45.0f) {
            ray_clear = proj;
          }
        }
      }
    }

    // Food attraction
    float food_score = 0.0f;
    for (int f = 0; f < w->num_foods; f++) {
      if (!w->foods[f].active) continue;
      float fd2 = sim_dist2(sim_x, sim_y, w->foods[f].x, w->foods[f].y);
      if (fd2 < (600.0f * 600.0f)) {
        food_score += w->foods[f].sz / (1.0f + sqrtf(fd2) * 0.005f);
      }
    }

    float turn_penalty = weights->weight_turn_penalty * (fabsf(sim_ang_diff(target_ang, s->ang)) / (float)M_PI);
    float boost_penalty = boost ? weights->weight_boost_cost : 0.0f;
    float score = weights->weight_clearance * (ray_clear / 800.0f) +
                  weights->weight_food * food_score - turn_penalty - boost_penalty;

    if (score > best_score) {
      best_score = score;
      best_ang = target_ang;
      best_boost = boost && (s->mass > 25.0f);
    }
  }

  s->target_ang = best_ang;
  s->boosting = best_boost;

  // Angle smoothing
  float diff = sim_ang_diff(s->target_ang, s->smooth_ang);
  s->smooth_ang += diff * (1.0f - expf(-weights->angle_smooth_rate * dt));
}

// Rival bots simple heuristics
static void sim_step_rival_ai(sim_world_t* w, sim_snake_t* s, float dt) {
  (void)dt;
  if (s->bot_type == BOT_TYPE_AGGRESSIVE) {
    // Attack closest other snake head
    float min_d2 = 1e9f;
    float tx = s->x + cosf(s->ang) * 200.0f;
    float ty = s->y + sinf(s->ang) * 200.0f;
    for (int i = 0; i < w->num_snakes; i++) {
      if (i == s->id || !w->snakes[i].alive) continue;
      float d2 = sim_dist2(s->x, s->y, w->snakes[i].x, w->snakes[i].y);
      if (d2 < min_d2) {
        min_d2 = d2;
        // Lead target ahead of victim
        tx = w->snakes[i].x + cosf(w->snakes[i].ang) * 80.0f;
        ty = w->snakes[i].y + sinf(w->snakes[i].ang) * 80.0f;
      }
    }
    s->target_ang = atan2f(ty - s->y, tx - s->x);
    s->boosting = (min_d2 < (350.0f * 350.0f)) && (s->mass > 25.0f);
  } else if (s->bot_type == BOT_TYPE_FARMER) {
    // Find closest food
    float min_fd2 = 1e9f;
    float fx = s->x, fy = s->y;
    for (int f = 0; f < w->num_foods; f++) {
      if (!w->foods[f].active) continue;
      float d2 = sim_dist2(s->x, s->y, w->foods[f].x, w->foods[f].y);
      if (d2 < min_fd2) {
        min_fd2 = d2;
        fx = w->foods[f].x;
        fy = w->foods[f].y;
      }
    }
    s->target_ang = atan2f(fy - s->y, fx - s->x);
    s->boosting = false;
  } else {
    // Wanderer: subtle random steering
    s->target_ang += sim_rng_float(&w->rng_state, -0.15f, 0.15f);
    s->boosting = false;
  }
}

sim_match_result_t sim_run_match(sim_world_t* w) {
  float dt = 0.05f; // 20 updates / sec
  float max_time = (w->cfg.max_duration_sec > 1.0f) ? w->cfg.max_duration_sec : 90.0f;

  while (w->sim_time < max_time && w->snakes[0].alive) {
    w->sim_time += dt;

    // 1. AI Decision Step
    for (int i = 0; i < w->num_snakes; i++) {
      sim_snake_t* s = &w->snakes[i];
      if (!s->alive) continue;
      if (s->bot_type == BOT_TYPE_SUBJECT) {
        sim_step_subject_ai(w, s, dt);
      } else {
        sim_step_rival_ai(w, s, dt);
      }
    }

    // 2. Kinematic Movement Step
    for (int i = 0; i < w->num_snakes; i++) {
      sim_snake_t* s = &w->snakes[i];
      if (!s->alive) continue;

      float v = s->boosting ? 13.5f : 5.78f;
      float max_omega = 5.2f * fminf(1.2f, 5.78f / v);

      float dang = sim_ang_diff(s->target_ang, s->ang);
      float step_turn = max_omega * dt;
      if (dang > step_turn) dang = step_turn;
      if (dang < -step_turn) dang = -step_turn;
      s->ang += dang;

      s->x += cosf(s->ang) * (v * dt);
      s->y += sinf(s->ang) * (v * dt);

      // Mass consumption in boost
      if (s->boosting) {
        s->mass -= 0.035f;
        if (s->mass < 15.0f) {
          s->mass = 15.0f;
          s->boosting = false;
        }
      }

      if (s->mass > s->max_mass) s->max_mass = s->mass;
      s->len = 15 + (int)(s->mass * 0.6f);
      if (s->len > MAX_SIM_PTS) s->len = MAX_SIM_PTS;

      // Trailing body segments update
      for (int p = s->len - 1; p > 0; p--) {
        s->pts_x[p] = s->pts_x[p - 1];
        s->pts_y[p] = s->pts_y[p - 1];
      }
      s->pts_x[0] = s->x;
      s->pts_y[0] = s->y;
      s->survival_time = w->sim_time;
    }

    // 3. Food Eating Step
    for (int i = 0; i < w->num_snakes; i++) {
      sim_snake_t* s = &w->snakes[i];
      if (!s->alive) continue;
      float eat_r = 14.0f + s->mass * 0.06f;
      for (int f = 0; f < w->num_foods; f++) {
        if (!w->foods[f].active) continue;
        if (sim_dist2(s->x, s->y, w->foods[f].x, w->foods[f].y) < eat_r * eat_r) {
          s->mass += w->foods[f].sz * 0.8f;
          w->foods[f].active = false;
          // Respawn food elsewhere to keep arena active
          float r = sqrtf(sim_rng_float(&w->rng_state, 0.0f, 1.0f)) * (w->arena_radius * 0.85f);
          float a = sim_rng_float(&w->rng_state, 0.0f, PI2);
          w->foods[f].x = cosf(a) * r;
          w->foods[f].y = sinf(a) * r;
          w->foods[f].sz = sim_rng_float(&w->rng_state, 1.0f, 3.5f);
          w->foods[f].active = true;
        }
      }
    }

    // 4. Collision Resolution Step
    for (int i = 0; i < w->num_snakes; i++) {
      sim_snake_t* s = &w->snakes[i];
      if (!s->alive) continue;

      // Border collision
      if (sqrtf(s->x * s->x + s->y * s->y) > w->arena_radius) {
        s->alive = false;
        s->death_cause = DEATH_CAUSE_BORDER;
        continue;
      }

      // Collisions with other snakes
      for (int j = 0; j < w->num_snakes; j++) {
        if (i == j) continue;
        sim_snake_t* other = &w->snakes[j];
        if (!other->alive) continue;

        // Head against other snake body
        float coll_rad = 12.0f + other->mass * 0.05f;
        for (int p = 0; p < other->len; p++) {
          if (sim_dist2(s->x, s->y, other->pts_x[p], other->pts_y[p]) < coll_rad * coll_rad) {
            s->alive = false;
            s->death_cause = DEATH_CAUSE_BODY;
            other->kills++;
            break;
          }
        }
        if (!s->alive) break;
      }
    }
  }

  sim_snake_t* sub = &w->snakes[0];
  sim_match_result_t res = {
    .seed = w->cfg.seed,
    .survival_time = sub->survival_time,
    .max_mass = sub->max_mass,
    .final_len = sub->len,
    .kills = sub->kills,
    .death_cause = sub->alive ? DEATH_CAUSE_NONE : sub->death_cause
  };

  // Optional telemetry JSONL logging
  if (w->cfg.jsonl_log_path) {
    FILE* fp = fopen(w->cfg.jsonl_log_path, "a");
    if (fp) {
      const char* cause_str = (res.death_cause == DEATH_CAUSE_BODY) ? "BODY_COLLISION" :
                              ((res.death_cause == DEATH_CAUSE_HEAD) ? "HEAD_COLLISION" :
                              ((res.death_cause == DEATH_CAUSE_BORDER) ? "MAP_BORDER" : "SURVIVED_TIME_LIMIT"));
      fprintf(fp, "{\"match_id\":%llu,\"duration_s\":%.2f,\"max_mass\":%.1f,\"final_len\":%d,\"kills\":%d,\"death_cause\":\"%s\"}\n",
              (unsigned long long)res.seed, res.survival_time, res.max_mass, res.final_len, res.kills, cause_str);
      fclose(fp);
    }
  }

  return res;
}
