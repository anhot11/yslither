#include "sbot.h"
#include "sbot_weights.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#include "../constants.h"
#include "../user.h"
#include "custom_controls.h"
#include "touch_input.h"

// ============================================================================
// yslither: Autonomous Game AI (sbot.c)
// Enhanced with state-of-the-art techniques from:
// - Hoobs Slither Mod (iteacher): Food Cluster Center-of-Mass, Density Scoring,
//   Polar Threat Heatmap with Velocity Prediction, Exclusion Zones (Anti-Trap)
// - Slither.io Auto-Play Genetic Heuristics: Safe Corridor Flood-Fill & Hysteresis
// - Real-Time Botstorm & Feast Harvesting
// ============================================================================

#define NUM_EVAL_DIRS 64
#define NUM_ROLLOUT_SUBSTEPS 4
#define MAX_COLL_PTS 2048
#define MAX_EXCLUSION_ZONES 8
#define SPEED_BASE 5.78f
#define SPEED_BOOST 13.5f

// Obstacle perception types
#define COLL_TYPE_HEAD 0
#define COLL_TYPE_ENEMY_BODY 1
#define COLL_TYPE_SELF_BODY 2
#define COLL_TYPE_BORDER 3

typedef struct {
  float x, y;
  float d2;
  float r;
  int type;
  int snake_idx;
  float ang;
  float speed;
} sbot_coll_pt;

typedef struct {
  float x, y;
  float dist;
  float sz;
  float value;
} sbot_food_cand;

typedef struct {
  float x, y;
  float radius;
  int frames_left;
  bool active;
} sbot_exclusion_zone_t;

typedef struct {
  float target_ang;
  bool accel;
  bool valid;
  float score;
  float clearance;
  float min_dist_coll;
  float end_x, end_y;
  float sim_path_x[NUM_ROLLOUT_SUBSTEPS];
  float sim_path_y[NUM_ROLLOUT_SUBSTEPS];
} sbot_rollout_t;

typedef struct {
  float dir_ang;
  float clearance;
  float score;
  bool valid;
  float threat_inner; // r < 240px
  float threat_mid;   // 240 <= r < 520px
  float threat_outer; // 520 <= r < 880px
} sbot_radar_ray_t;

typedef struct {
  int stage;                     // 0 = FARM, 1 = HUNT, 2 = ESCAPE, 3 = COIL
  int escape_frames_clear;
  int hunt_target_id;
  int coil_dir;                  // +1 or -1

  int id;
  float x, y, ang;
  float speed;
  float radius;
  float width;
  int snake_len;
  float score_val;

  // Smoothing
  float smooth_ang;
  bool smooth_ang_initialized;

  // Output steering
  float target_x, target_y;
  bool target_accel;

  // Perception buffer (zero dynamic allocation)
  sbot_coll_pt coll_pts[MAX_COLL_PTS];
  int coll_pts_n;

  // Food cluster focus (Center-of-Mass & Density)
  sbot_food_cand best_food;
  bool has_food;

  // Exclusion Zones (Anti-Trap Memory)
  sbot_exclusion_zone_t exclusion_zones[MAX_EXCLUSION_ZONES];

  // Rollouts & Radar (64 directions)
  sbot_rollout_t rollouts[NUM_EVAL_DIRS * 2]; // 64 normal + 64 turbo = 128
  sbot_radar_ray_t radar[NUM_EVAL_DIRS];
  int best_rollout_idx;

  // Tunable weights
  sbot_weights_t weights;
} sbot_engine_t;

static sbot_engine_t B;

// ----------------------------------------------------------------------------
// Utilities
// ----------------------------------------------------------------------------

static inline float dist2(float ax, float ay, float bx, float by) {
  float dx = ax - bx, dy = ay - by;
  return dx * dx + dy * dy;
}

static inline float ang_between(float a, float b) {
  float d = fmodf(a - b, PI2);
  if (d < -(float)M_PI) d += PI2;
  if (d > (float)M_PI) d -= PI2;
  return d;
}

static inline float snake_width_calc(float sc) {
  return roundf(sc * 29.0f);
}

static inline bool is_feeder_snake(const snake* s) {
  if (!s) return false;
  return strncmp(s->nk, "[FEED]", 6) == 0;
}

static void sbot_add_exclusion_zone(float x, float y, float radius, int duration_frames) {
  int slot = -1;
  int min_frames = 999999;
  for (int i = 0; i < MAX_EXCLUSION_ZONES; i++) {
    if (!B.exclusion_zones[i].active) {
      slot = i;
      break;
    }
    if (B.exclusion_zones[i].frames_left < min_frames) {
      min_frames = B.exclusion_zones[i].frames_left;
      slot = i;
    }
  }
  if (slot >= 0) {
    B.exclusion_zones[slot] = (sbot_exclusion_zone_t){
      .x = x, .y = y, .radius = radius, .frames_left = duration_frames, .active = true
    };
  }
}

// ----------------------------------------------------------------------------
// Perception Pipeline
// ----------------------------------------------------------------------------

static void sbot_perceive_world(game_data* gdata) {
  B.coll_pts_n = 0;
  B.has_food = false;

  // Update and decay exclusion zones
  for (int i = 0; i < MAX_EXCLUSION_ZONES; i++) {
    if (B.exclusion_zones[i].active) {
      B.exclusion_zones[i].frames_left--;
      if (B.exclusion_zones[i].frames_left <= 0) {
        B.exclusion_zones[i].active = false;
      }
    }
  }

  float world_rad = gdata->data.flux_grd > 1000.0f ? gdata->data.flux_grd : 21600.0f;
  float world_cx = gdata->data.grd > 1000.0f ? gdata->data.grd : 21600.0f;
  float world_cy = world_cx;

  // 1. Map Border Representation (ring of repulsive sentinel points)
  float dist_to_center = sqrtf(dist2(B.x, B.y, world_cx, world_cy));
  if (dist_to_center > (world_rad - 1800.0f)) {
    float base_ang = atan2f(B.y - world_cy, B.x - world_cx);
    for (float dang = -0.8f; dang <= 0.8f; dang += 0.2f) {
      float ang = base_ang + dang;
      float bx = world_cx + cosf(ang) * world_rad;
      float by = world_cy + sinf(ang) * world_rad;
      float d2 = dist2(B.x, B.y, bx, by);
      if (B.coll_pts_n < MAX_COLL_PTS) {
        B.coll_pts[B.coll_pts_n++] = (sbot_coll_pt){
          .x = bx, .y = by, .d2 = d2, .r = 45.0f,
          .type = COLL_TYPE_BORDER, .snake_idx = -1, .ang = 0, .speed = 0
        };
      }
    }
  }

  // 2. Snake Obstacles (Bodies and Heads)
  int ns = tdarray_length(gdata->data.snakes);
  for (int i = 0; i < ns; i++) {
    snake* s = gdata->data.snakes + i;
    if (s->dead || is_feeder_snake(s)) continue;

    float s_rad = snake_width_calc(s->sc) * 0.5f;

    // Rival Heads (Velocity projection and lethal danger zones)
    if (s->id != B.id) {
      float hx = s->xx;
      float hy = s->yy;
      float hd2 = dist2(B.x, B.y, hx, hy);

      if (hd2 < (1400.0f * 1400.0f)) {
        if (B.coll_pts_n < MAX_COLL_PTS) {
          B.coll_pts[B.coll_pts_n++] = (sbot_coll_pt){
            .x = hx, .y = hy, .d2 = hd2, .r = s_rad * 1.5f,
            .type = COLL_TYPE_HEAD, .snake_idx = i, .ang = s->ang, .speed = s->sp
          };
        }
      }
    }

    // Bodies: Continuous segment sampling with distance culling
    int pn = tdarray_length(s->pts);
    bool is_self = (s->id == B.id);
    int start_node = is_self ? 12 : 0;

    for (int j = start_node; j < pn; j++) {
      body_part* bp = s->pts + j;
      if (bp->dying) continue;

      float pd2 = dist2(B.x, B.y, bp->xx, bp->yy);
      if (pd2 > (1200.0f * 1200.0f)) continue;

      if (pd2 > (800.0f * 800.0f) && (j % 2 != 0)) continue;

      if (B.coll_pts_n < MAX_COLL_PTS) {
        B.coll_pts[B.coll_pts_n++] = (sbot_coll_pt){
          .x = bp->xx, .y = bp->yy, .d2 = pd2, .r = s_rad * 1.35f,
          .type = is_self ? COLL_TYPE_SELF_BODY : COLL_TYPE_ENEMY_BODY,
          .snake_idx = i, .ang = 0, .speed = 0
        };
      }
    }
  }

  // 3. Center-of-Mass & Density Food Clustering (Hoobs / iteacher algorithm)
  int nf = tdarray_length(gdata->data.foods);
  if (nf > 0) {
    sbot_food_cand pool[96];
    int pool_n = 0;
    for (int i = 0; i < nf && pool_n < 96; i++) {
      food* f = gdata->data.foods + i;
      float fd2 = dist2(B.x, B.y, f->xx, f->yy);
      if (fd2 > (1100.0f * 1100.0f)) continue;
      float sz = f->sz;
      float val = 1.0f + (sz >= 2.5f ? sz * 10.0f : sz * 1.2f);
      pool[pool_n++] = (sbot_food_cand){
        .x = f->xx, .y = f->yy, .dist = sqrtf(fd2), .sz = sz, .value = val
      };
    }

    if (pool_n > 0) {
      const float CLUSTER_RADIUS = 220.0f;
      const float CLUSTER_R2 = CLUSTER_RADIUS * CLUSTER_RADIUS;
      bool used[96] = {false};

      float best_cluster_score = -1e9f;
      sbot_food_cand best_target = {0};

      for (int i = 0; i < pool_n; i++) {
        if (used[i]) continue;
        used[i] = true;

        float x_sum = pool[i].x * pool[i].value;
        float y_sum = pool[i].y * pool[i].value;
        float val_sum = pool[i].value;
        int count = 1;

        for (int j = i + 1; j < pool_n; j++) {
          if (used[j]) continue;
          if (dist2(pool[i].x, pool[i].y, pool[j].x, pool[j].y) <= CLUSTER_R2) {
            used[j] = true;
            x_sum += pool[j].x * pool[j].value;
            y_sum += pool[j].y * pool[j].value;
            val_sum += pool[j].value;
            count++;
          }
        }

        float cx = x_sum / val_sum;
        float cy = y_sum / val_sum;
        float d_to_c = sqrtf(dist2(B.x, B.y, cx, cy));

        // Skip clusters inside active exclusion zones
        bool in_exclusion = false;
        for (int z = 0; z < MAX_EXCLUSION_ZONES; z++) {
          if (B.exclusion_zones[z].active) {
            float ez_d2 = dist2(cx, cy, B.exclusion_zones[z].x, B.exclusion_zones[z].y);
            float ez_r = B.exclusion_zones[z].radius;
            if (ez_d2 < ez_r * ez_r) {
              in_exclusion = true;
              break;
            }
          }
        }
        if (in_exclusion) continue;

        // Safety filter near enemy bodies/heads
        float risk = 1.0f;
        for (int k = 0; k < B.coll_pts_n; k++) {
          sbot_coll_pt* cp = &B.coll_pts[k];
          float cd2 = dist2(cx, cy, cp->x, cp->y);
          if (cd2 < (cp->r + 45.0f) * (cp->r + 45.0f)) {
            risk += 14.0f;
            break;
          }
        }

        float density = val_sum / CLUSTER_R2;
        float c_ang = atan2f(cy - B.y, cx - B.x);
        float d_head = fabsf(ang_between(c_ang, B.ang));
        float forward_mult = (val_sum > 15.0f) ? 1.0f : (1.0f + 0.7f * cosf(d_head));

        float score = (((val_sum / (d_to_c + 1.0f)) + (density * 12.0f)) * forward_mult) / risk;

        if (score > best_cluster_score) {
          best_cluster_score = score;
          best_target = (sbot_food_cand){
            .x = cx, .y = cy, .dist = d_to_c, .sz = (float)count, .value = val_sum
          };
          B.has_food = true;
        }
      }

      if (B.has_food) {
        B.best_food = best_target;
      }
    }
  }
}

// ----------------------------------------------------------------------------
// Kinematic Rollout Simulator (128 rollouts: 64 directions x {cruise, turbo})
// ----------------------------------------------------------------------------

static void sbot_evaluate_kinematic_rollouts(game_data* gdata) {
  float dt_sub[NUM_ROLLOUT_SUBSTEPS] = {0.15f, 0.20f, 0.25f, 0.35f};
  float world_rad = gdata->data.flux_grd > 1000.0f ? gdata->data.flux_grd : 21600.0f;
  float world_cx = gdata->data.grd > 1000.0f ? gdata->data.grd : 21600.0f;
  float world_cy = world_cx;

  float safety_margin = B.weights.safety_margin_base + B.weights.adaptive_safety_delta;
  if (safety_margin < 15.0f) safety_margin = 15.0f;

  int total_candidates = NUM_EVAL_DIRS * 2;
  B.best_rollout_idx = 0;
  float highest_score = -1e9f;

  // Initialize radar threat channels
  for (int d = 0; d < NUM_EVAL_DIRS; d++) {
    B.radar[d].threat_inner = 0.0f;
    B.radar[d].threat_mid = 0.0f;
    B.radar[d].threat_outer = 0.0f;
  }

  // Populate Polar Threat Heatmap channels from obstacles
  for (int k = 0; k < B.coll_pts_n; k++) {
    sbot_coll_pt* cp = &B.coll_pts[k];
    float d = sqrtf(cp->d2);
    float ang = atan2f(cp->y - B.y, cp->x - B.x);
    if (ang < 0.0f) ang += PI2;
    int sector = (int)(ang * (NUM_EVAL_DIRS / PI2)) % NUM_EVAL_DIRS;

    float weight = (cp->type == COLL_TYPE_HEAD) ? 4.0f : 1.5f;
    if (d < 240.0f) B.radar[sector].threat_inner += weight;
    else if (d < 520.0f) B.radar[sector].threat_mid += weight;
    else if (d < 880.0f) B.radar[sector].threat_outer += weight;
  }

  for (int cand_idx = 0; cand_idx < total_candidates; cand_idx++) {
    int dir_idx = cand_idx % NUM_EVAL_DIRS;
    bool boost_mode = (cand_idx >= NUM_EVAL_DIRS);

    float target_ang = (float)dir_idx * (PI2 / (float)NUM_EVAL_DIRS);
    float v = boost_mode ? SPEED_BOOST : SPEED_BASE;
    float max_omega = B.weights.max_turn_rate_base * fminf(1.2f, 5.78f / v);

    sbot_rollout_t* ro = &B.rollouts[cand_idx];
    ro->target_ang = target_ang;
    ro->accel = boost_mode;
    ro->valid = true;
    ro->score = 0.0f;
    ro->min_dist_coll = 9999.0f;

    float sim_x = B.x;
    float sim_y = B.y;
    float sim_ang = B.ang;
    float sim_time = 0.0f;

    float kill_opportunity_bonus = 0.0f;

    for (int step = 0; step < NUM_ROLLOUT_SUBSTEPS; step++) {
      float dt = dt_sub[step];
      sim_time += dt;

      float d_ang = ang_between(target_ang, sim_ang);
      float max_step_turn = max_omega * dt;
      if (d_ang > max_step_turn) d_ang = max_step_turn;
      if (d_ang < -max_step_turn) d_ang = -max_step_turn;
      sim_ang += d_ang;

      sim_x += cosf(sim_ang) * (v * dt);
      sim_y += sinf(sim_ang) * (v * dt);

      ro->sim_path_x[step] = sim_x;
      ro->sim_path_y[step] = sim_y;

      // 1. World Border Collision Check
      float dist_center = sqrtf(dist2(sim_x, sim_y, world_cx, world_cy));
      if (dist_center + B.radius + safety_margin > (world_rad - 150.0f)) {
        ro->valid = false;
        ro->score = -1e8f;
        break;
      }

      // 2. Obstacle Collisions Check
      for (int k = 0; k < B.coll_pts_n; k++) {
        sbot_coll_pt* cp = &B.coll_pts[k];
        float obs_x = cp->x;
        float obs_y = cp->y;

        if (cp->type == COLL_TYPE_HEAD) {
          obs_x += cosf(cp->ang) * (cp->speed * sim_time);
          obs_y += sinf(cp->ang) * (cp->speed * sim_time);
        }

        float d_obs = sqrtf(dist2(sim_x, sim_y, obs_x, obs_y));
        if (d_obs < ro->min_dist_coll) ro->min_dist_coll = d_obs;

        float required_clearance = B.radius + cp->r + safety_margin;

        if (cp->type == COLL_TYPE_HEAD) {
          float rival_scale = 1.0f;
          if (cp->snake_idx >= 0 && cp->snake_idx < tdarray_length(gdata->data.snakes)) {
            rival_scale = gdata->data.snakes[cp->snake_idx].sc;
          }
          if (rival_scale >= (B.width / 29.0f) * 0.95f) {
            required_clearance += 40.0f;
          } else {
            if (d_obs > (B.radius + cp->r + 15.0f) && d_obs < (B.radius + cp->r + 120.0f)) {
              kill_opportunity_bonus += B.weights.weight_hunt_cut * 15.0f;
            }
          }
        }

        if (d_obs < required_clearance) {
          ro->valid = false;
          ro->score = -1e8f;
          break;
        }
      }

      if (!ro->valid) break;
    }

    ro->end_x = sim_x;
    ro->end_y = sim_y;

    if (!ro->valid) continue;

    // Raycast corridor clearance
    float ray_clearance = 1100.0f;
    float ray_dx = cosf(sim_ang);
    float ray_dy = sinf(sim_ang);

    for (int k = 0; k < B.coll_pts_n; k++) {
      sbot_coll_pt* cp = &B.coll_pts[k];
      float vx = cp->x - sim_x;
      float vy = cp->y - sim_y;
      float proj = vx * ray_dx + vy * ray_dy;
      if (proj > 0.0f && proj < ray_clearance) {
        float perp2 = (vx * vx + vy * vy) - (proj * proj);
        float req_r = cp->r + B.radius + safety_margin;
        if (perp2 < req_r * req_r) {
          ray_clearance = proj;
        }
      }
    }
    ro->clearance = ray_clearance;

    float clearance_score = B.weights.weight_clearance * (fminf(ray_clearance, 1000.0f) / 1000.0f);

    float food_score = 0.0f;
    if (B.has_food) {
      float d_to_food = sqrtf(dist2(sim_x, sim_y, B.best_food.x, B.best_food.y));
      food_score = B.weights.weight_food * (B.best_food.value / (1.0f + d_to_food * 0.0035f));
    }

    float turn_diff = fabsf(ang_between(target_ang, B.ang));
    float turn_penalty = B.weights.weight_turn_penalty * (turn_diff / (float)M_PI);

    float border_penalty = 0.0f;
    float dist_ctr_end = sqrtf(dist2(sim_x, sim_y, world_cx, world_cy));
    if (dist_ctr_end > (world_rad - 1600.0f)) {
      float border_depth = (dist_ctr_end - (world_rad - 1600.0f)) / 1600.0f;
      border_penalty = B.weights.weight_border_repulse * border_depth * border_depth;
    }

    // Exclusion zones penalty (anti-trap memory)
    float exclusion_penalty = 0.0f;
    for (int z = 0; z < MAX_EXCLUSION_ZONES; z++) {
      if (B.exclusion_zones[z].active) {
        float ez_d2 = dist2(sim_x, sim_y, B.exclusion_zones[z].x, B.exclusion_zones[z].y);
        float ez_r = B.exclusion_zones[z].radius;
        if (ez_d2 < ez_r * ez_r) {
          exclusion_penalty += 35.0f * (1.0f - sqrtf(ez_d2) / ez_r);
        }
      }
    }

    float boost_penalty = boost_mode ? B.weights.weight_boost_cost : 0.0f;

    ro->score = clearance_score + food_score + kill_opportunity_bonus - turn_penalty - border_penalty - exclusion_penalty - boost_penalty;

    if (!boost_mode) {
      B.radar[dir_idx].dir_ang = target_ang;
      B.radar[dir_idx].clearance = ray_clearance;
      B.radar[dir_idx].score = ro->score;
      B.radar[dir_idx].valid = ro->valid;
    }

    if (ro->score > highest_score) {
      highest_score = ro->score;
      B.best_rollout_idx = cand_idx;
    }
  }

  // Fallback: If ALL rollouts collide, select candidate with maximum time-to-impact
  if (highest_score <= -1e7f) {
    float max_escape_dist = -1.0f;
    for (int i = 0; i < total_candidates; i++) {
      if (B.rollouts[i].min_dist_coll > max_escape_dist) {
        max_escape_dist = B.rollouts[i].min_dist_coll;
        B.best_rollout_idx = i;
      }
    }
  }
}

// ----------------------------------------------------------------------------
// Hysteresis State Machine (FARM, HUNT, ESCAPE, COIL)
// ----------------------------------------------------------------------------

static void sbot_update_state_machine(game_data* gdata, user_settings* usrs) {
  (void)gdata;
  sbot_rollout_t* best_ro = &B.rollouts[B.best_rollout_idx];

  if (usrs->bot_mode == 2 || (B.score_val > usrs->bot_follow_circle_score && usrs->bot_follow_circle_score > 0)) {
    B.stage = 3; // COIL
    return;
  }

  float forward_clearance = best_ro->clearance;

  if (B.stage == 2) {
    if (forward_clearance > B.weights.escape_exit_dist) {
      B.escape_frames_clear++;
      if (B.escape_frames_clear >= B.weights.escape_min_frames) {
        B.stage = 0; // Return to FARM
        B.escape_frames_clear = 0;
      }
    } else {
      B.escape_frames_clear = 0;
    }
  } else {
    if (forward_clearance < B.weights.escape_enter_dist) {
      B.stage = 2; // ESCAPE
      B.escape_frames_clear = 0;
      // Mark current location as exclusion zone so we don't turn back into the trap
      sbot_add_exclusion_zone(B.x, B.y, 350.0f, 180); // ~3 seconds @ 60 FPS
    } else if (usrs->bot_mode == 1 && B.has_food && B.best_food.value >= 15.0f) {
      B.stage = 1; // HUNT (Feast or predatory intercept)
    } else {
      B.stage = 0; // FARM
    }
  }
}

// ----------------------------------------------------------------------------
// Public API Implementations
// ----------------------------------------------------------------------------

void sbot_init(tenv* env) {
  (void)env;
  memset(&B, 0, sizeof(B));
  sbot_weights_init_default(&B.weights);
  B.smooth_ang_initialized = false;
  B.coil_dir = 1;
}

void sbot_go(tenv* env) {
  tuser_data* usr = (tuser_data*)env->usr;
  if (!usr) return;

  user_settings* usrs = &usr->usrs;
  game_data* gdata = &usr->gdata;
  sbot* bot = &gdata->bot;

  if (!usrs->hotkeys[HOTKEY_BOT].active) return;
  if (tdarray_length(gdata->data.snakes) == 0) return;

  snake* me = gdata->data.snakes + 0;
  if (!me || me->dead) return;

  // 1. Sync self telemetry
  B.id = me->id;
  B.x = me->xx + me->fx;
  B.y = me->yy + me->fy;
  B.ang = me->ang;
  B.speed = me->sp;
  B.snake_len = tdarray_length(me->pts);
  B.width = snake_width_calc(me->sc);
  B.radius = B.width * 0.5f;
  B.score_val = (float)me->sct;

  if (!B.smooth_ang_initialized) {
    B.smooth_ang = B.ang;
    B.smooth_ang_initialized = true;
  }

  // 2. Perception pass with Center-of-Mass & Density Food Clustering
  sbot_perceive_world(gdata);

  // 3. 128 Kinematic Rollouts Evaluation (64 directions x {cruise, turbo})
  sbot_evaluate_kinematic_rollouts(gdata);

  // 4. Hysteresis Mode Update & Exclusion Zone generation
  sbot_update_state_machine(gdata, usrs);

  // 5. Select Best Direction & Mass-Positive Turbo
  sbot_rollout_t* best_ro = &B.rollouts[B.best_rollout_idx];
  float target_ang = best_ro->target_ang;
  bool use_turbo = false;

  if (usrs->bot_auto_turbo) {
    if (B.stage == 2) {
      if (best_ro->clearance > 320.0f && best_ro->min_dist_coll < 180.0f) {
        use_turbo = true;
      }
    } else if (B.stage == 1) {
      if (B.has_food && B.best_food.value > 15.0f && best_ro->clearance > 450.0f) {
        float f_diff = fabsf(ang_between(atan2f(B.best_food.y - B.y, B.best_food.x - B.x), B.ang));
        if (f_diff < ((float)M_PI * 0.22f)) {
          use_turbo = true;
        }
      }
    }
  }

  if (B.snake_len < 20) {
    use_turbo = false;
  }

  if (custom_controls_is_boost_active() || touch_input_is_boosting()) {
    use_turbo = true;
  }

  // 6. Continuous Exponential Angle Smoothing (Zero Jitter)
  float dt = gdata->data.etm * 0.001f;
  if (dt <= 0.0f || dt > 0.1f) dt = 0.016666f;

  float d_target = ang_between(target_ang, B.smooth_ang);
  float smooth_blend = 1.0f - expf(-B.weights.angle_smooth_rate * dt);
  B.smooth_ang += d_target * smooth_blend;

  bot->output.xm = (int)roundf(cosf(B.smooth_ang) * 250.0f);
  bot->output.ym = (int)roundf(sinf(B.smooth_ang) * 250.0f);
  bot->output.accel = use_turbo;

  B.target_x = B.x + cosf(B.smooth_ang) * 350.0f;
  B.target_y = B.y + sinf(B.smooth_ang) * 350.0f;
  B.target_accel = use_turbo;
}

// ----------------------------------------------------------------------------
// Visual Debug Overlay (ImGui 64-Direction Polar Threat Heatmap & Reticle)
// ----------------------------------------------------------------------------

void sbot_render_overlay(tenv* env) {
  tuser_data* usr = (tuser_data*)env->usr;
  if (!usr) return;

  user_settings* usrs = &usr->usrs;
  game_data* gdata = &usr->gdata;
  tcontext* ctx = env->ctx;

  if (!usrs->debug_logs_enabled) return;
  if (!usrs->hotkeys[HOTKEY_BOT].active) return;
  if (tdarray_length(gdata->data.snakes) == 0) return;

  ImDrawList* dl = igGetWindowDrawList();
  if (!dl) return;

  float mww2 = ctx->size[0] * 0.5f;
  float mhh2 = ctx->size[1] * 0.5f;
  float gsc = gdata->data.gsc;
  float view_xx = gdata->data.view_xx;
  float view_yy = gdata->data.view_yy;

  float head_sx = mww2 + (B.x - view_xx) * gsc;
  float head_sy = mhh2 + (B.y - view_yy) * gsc;

  // 1. Concentric Polar Threat Radar Rings (Hoobs / iteacher architecture)
  if (usrs->bot_visual_radar) {
    // 3 Concentric rings
    float r_in = 180.0f * gsc;
    float r_mid = 380.0f * gsc;
    float r_out = 600.0f * gsc;

    ImDrawList_AddCircle(dl, (ImVec2){head_sx, head_sy}, r_in,
                         igColorConvertFloat4ToU32((ImVec4){1.0f, 0.2f, 0.2f, 0.25f}), 32, 1.0f);
    ImDrawList_AddCircle(dl, (ImVec2){head_sx, head_sy}, r_mid,
                         igColorConvertFloat4ToU32((ImVec4){1.0f, 0.8f, 0.2f, 0.20f}), 32, 1.0f);
    ImDrawList_AddCircle(dl, (ImVec2){head_sx, head_sy}, r_out,
                         igColorConvertFloat4ToU32((ImVec4){0.2f, 0.8f, 1.0f, 0.15f}), 32, 1.0f);

    for (int i = 0; i < NUM_EVAL_DIRS; i++) {
      sbot_radar_ray_t* ray = &B.radar[i];
      float r_len = fminf(ray->clearance, 420.0f) * gsc;
      if (r_len < 10.0f) r_len = 10.0f;

      float rx = head_sx + cosf(ray->dir_ang) * r_len;
      float ry = head_sy + sinf(ray->dir_ang) * r_len;

      ImVec4 col;
      if (!ray->valid) {
        col = (ImVec4){0.95f, 0.15f, 0.15f, 0.35f};
      } else if (ray->threat_inner > 0.0f) {
        col = (ImVec4){1.0f, 0.35f, 0.15f, 0.55f}; // Amber/Orange inner danger
      } else if (ray->clearance > 600.0f) {
        col = (ImVec4){0.15f, 0.95f, 0.35f, 0.65f}; // Green open space
      } else {
        col = (ImVec4){0.95f, 0.85f, 0.15f, 0.50f}; // Yellow caution
      }

      ImDrawList_AddLine(dl, (ImVec2){head_sx, head_sy}, (ImVec2){rx, ry},
                         igColorConvertFloat4ToU32(col), 1.5f);
    }
  }

  // 2. Active Exclusion Zones (Anti-Trap Memory)
  for (int z = 0; z < MAX_EXCLUSION_ZONES; z++) {
    if (B.exclusion_zones[z].active) {
      float zx = mww2 + (B.exclusion_zones[z].x - view_xx) * gsc;
      float zy = mhh2 + (B.exclusion_zones[z].y - view_yy) * gsc;
      float zr = B.exclusion_zones[z].radius * gsc;
      ImDrawList_AddCircle(dl, (ImVec2){zx, zy}, zr,
                           igColorConvertFloat4ToU32((ImVec4){1.0f, 0.5f, 0.0f, 0.40f}), 24, 2.0f);
    }
  }

  // 3. Chosen Trajectory & Goal Reticle
  if (usrs->bot_visual_line) {
    sbot_rollout_t* best_ro = &B.rollouts[B.best_rollout_idx];

    float prev_x = head_sx;
    float prev_y = head_sy;
    for (int step = 0; step < NUM_ROLLOUT_SUBSTEPS; step++) {
      float px = mww2 + (best_ro->sim_path_x[step] - view_xx) * gsc;
      float py = mhh2 + (best_ro->sim_path_y[step] - view_yy) * gsc;

      ImDrawList_AddLine(dl, (ImVec2){prev_x, prev_y}, (ImVec2){px, py},
                         igColorConvertFloat4ToU32((ImVec4){0.0f, 0.90f, 1.0f, 0.85f}), 2.5f);
      prev_x = px;
      prev_y = py;
    }

    ImDrawList_AddCircle(dl, (ImVec2){prev_x, prev_y}, 12.0f,
                         igColorConvertFloat4ToU32((ImVec4){0.0f, 1.0f, 0.90f, 0.90f}), 16, 2.0f);
    ImDrawList_AddCircleFilled(dl, (ImVec2){prev_x, prev_y}, 3.0f,
                               igColorConvertFloat4ToU32((ImVec4){1.0f, 1.0f, 1.0f, 1.0f}), 8);
  }

  // 4. Target Food Cluster (Center-of-Mass & Total Feast Value)
  if (usrs->bot_visual_food && B.has_food) {
    float fx = mww2 + (B.best_food.x - view_xx) * gsc;
    float fy = mhh2 + (B.best_food.y - view_yy) * gsc;

    ImDrawList_AddLine(dl, (ImVec2){head_sx, head_sy}, (ImVec2){fx, fy},
                       igColorConvertFloat4ToU32((ImVec4){0.20f, 1.0f, 0.40f, 0.65f}), 1.8f);
    ImDrawList_AddCircle(dl, (ImVec2){fx, fy}, 14.0f,
                         igColorConvertFloat4ToU32((ImVec4){0.30f, 1.0f, 0.50f, 0.85f}), 16, 2.0f);
    ImDrawList_AddCircleFilled(dl, (ImVec2){fx, fy}, 4.0f,
                               igColorConvertFloat4ToU32((ImVec4){0.60f, 1.0f, 0.70f, 1.0f}), 8);
  }

  // 5. HUD State Badge
  const char* mode_name = "FARM";
  if (B.stage == 1) mode_name = "HUNT (FEAST)";
  else if (B.stage == 2) mode_name = "ESCAPE";
  else if (B.stage == 3) mode_name = "COIL";

  char badge[140];
  snprintf(badge, sizeof(badge), "BOT TOP [%s] | 64-Dir Rollouts | Clear: %.0fpx | Feast: %.0f | Turbo: %s",
           mode_name, B.rollouts[B.best_rollout_idx].clearance,
           B.has_food ? B.best_food.value : 0.0f,
           B.target_accel ? "ON" : "OFF");

  ImVec2 bsz;
  igCalcTextSize(&bsz, badge, NULL, false, -1);
  float bx = 16.0f;
  float by = 68.0f;

  ImDrawList_AddRectFilled(dl, (ImVec2){bx - 8.0f, by - 4.0f},
                           (ImVec2){bx + bsz.x + 8.0f, by + bsz.y + 4.0f},
                           igColorConvertFloat4ToU32((ImVec4){0.05f, 0.08f, 0.12f, 0.88f}), 6.0f, 0);
  ImDrawList_AddRect(dl, (ImVec2){bx - 8.0f, by - 4.0f},
                     (ImVec2){bx + bsz.x + 8.0f, by + bsz.y + 4.0f},
                     igColorConvertFloat4ToU32((ImVec4){0.0f, 0.80f, 1.0f, 0.75f}), 6.0f, 0, 1.5f);
  ImDrawList_AddText_Vec2(dl, (ImVec2){bx, by},
                          igColorConvertFloat4ToU32((ImVec4){0.20f, 0.95f, 1.0f, 1.0f}),
                          badge, NULL);
}

void sbot_destroy(tenv* env) {
  (void)env;
}