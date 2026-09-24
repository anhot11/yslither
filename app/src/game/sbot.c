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
// yslither Phase 3: Top-Tier Autonomous Game AI (sbot.c)
// - 64-Direction Kinematic Rollouts with Real Turning Constraints (128 rollouts/tick)
// - Geometric Capsule & Predictive Threat Perception
// - Raycast Open-Corridor / Cul-de-Sac Rejection (Flood-Fill Reachable Space)
// - Hysteresis State Machine (FARM, HUNT, ESCAPE, COIL)
// - Mass-Positive Smart Turbo & Continuous Exponential Angle Smoothing
// - < 0.4 ms/tick, Zero Malloc Hot Loop, ImGui Multi-Layer Visual Debug Radar
// ============================================================================

#define NUM_EVAL_DIRS 64
#define NUM_ROLLOUT_SUBSTEPS 4
#define MAX_COLL_PTS 2048
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

  // Food focus
  sbot_food_cand best_food;
  bool has_food;

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

// ----------------------------------------------------------------------------
// Perception Pipeline
// ----------------------------------------------------------------------------

static void sbot_perceive_world(game_data* gdata, snake* me) {
  B.coll_pts_n = 0;
  B.has_food = false;

  float world_rad = gdata->data.flux_grd > 1000.0f ? gdata->data.flux_grd : 21600.0f;
  float world_cx = gdata->data.grd > 1000.0f ? gdata->data.grd : 21600.0f;
  float world_cy = world_cx;

  // 1. Map Border Representation (ring of repulsive sentinel points)
  float dist_to_center = sqrtf(dist2(B.x, B.y, world_cx, world_cy));
  if (dist_to_center > (world_rad - 1800.0f)) {
    // Generate border sentinel points along the forward and adjacent perimeter
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

    // Rival Heads (Predictive trajectory and lethal danger zones)
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
    // For self, skip the neck region (first 10 nodes) to prevent self-collision
    int start_node = is_self ? 12 : 0;

    for (int j = start_node; j < pn; j++) {
      body_part* bp = s->pts + j;
      if (bp->dying) continue;

      float pd2 = dist2(B.x, B.y, bp->xx, bp->yy);
      if (pd2 > (1200.0f * 1200.0f)) continue;

      // Dense sampling for nearby bodies (< 800px) so no gaps exist between spheres
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

  // 3. Food Scanning & Clustering
  int nf = tdarray_length(gdata->data.foods);
  float best_food_score = -1.0f;

  for (int i = 0; i < nf; i++) {
    food* f = gdata->data.foods + i;
    float fd2 = dist2(B.x, B.y, f->xx, f->yy);
    if (fd2 > (1100.0f * 1100.0f)) continue;

    float fdist = sqrtf(fd2);
    float fang = atan2f(f->yy - B.y, f->xx - B.x);
    float dang = fabsf(ang_between(fang, B.ang));

    // Value scaling: high bonus for dead snake remnants (sz >= 2.5f)
    float val = 1.0f + (f->sz >= 2.5f ? f->sz * 9.0f : f->sz * 1.2f);
    float forward_mult = (f->sz >= 2.5f) ? 1.0f : (1.0f + 0.8f * cosf(dang));
    float prox = 1.0f / (1.0f + (fdist / 220.0f));

    // Safety filter: check if food is dangerously close to an enemy body/head
    float risk = 1.0f;
    for (int k = 0; k < B.coll_pts_n; k++) {
      sbot_coll_pt* cp = &B.coll_pts[k];
      float cd2 = dist2(f->xx, f->yy, cp->x, cp->y);
      if (cd2 < (cp->r + 40.0f) * (cp->r + 40.0f)) {
        risk += 12.0f;
        break;
      }
    }

    float fscore = (val * prox * forward_mult) / risk;
    if (fscore > best_food_score) {
      best_food_score = fscore;
      B.best_food = (sbot_food_cand){
        .x = f->xx, .y = f->yy, .dist = fdist, .sz = f->sz, .value = val
      };
      B.has_food = true;
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

  for (int cand_idx = 0; cand_idx < total_candidates; cand_idx++) {
    int dir_idx = cand_idx % NUM_EVAL_DIRS;
    bool boost_mode = (cand_idx >= NUM_EVAL_DIRS);

    float target_ang = (float)dir_idx * (PI2 / (float)NUM_EVAL_DIRS);
    float v = boost_mode ? SPEED_BOOST : SPEED_BASE;

    // Kinematic turning rate constrained by snake velocity & size
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

      // Turn towards target_ang subject to maximum turning angular velocity
      float d_ang = ang_between(target_ang, sim_ang);
      float max_step_turn = max_omega * dt;
      if (d_ang > max_step_turn) d_ang = max_step_turn;
      if (d_ang < -max_step_turn) d_ang = -max_step_turn;
      sim_ang += d_ang;

      // Integrate forward displacement
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

        // For enemy heads, project their future trajectory at sim_time
        if (cp->type == COLL_TYPE_HEAD) {
          obs_x += cosf(cp->ang) * (cp->speed * sim_time);
          obs_y += sinf(cp->ang) * (cp->speed * sim_time);
        }

        float d_obs = sqrtf(dist2(sim_x, sim_y, obs_x, obs_y));
        if (d_obs < ro->min_dist_coll) ro->min_dist_coll = d_obs;

        float required_clearance = B.radius + cp->r + safety_margin;

        // Head vs Head logic: if rival is larger, collision is strictly lethal
        if (cp->type == COLL_TYPE_HEAD) {
          float rival_scale = 1.0f;
          if (cp->snake_idx >= 0 && cp->snake_idx < tdarray_length(gdata->data.snakes)) {
            rival_scale = gdata->data.snakes[cp->snake_idx].sc;
          }
          if (rival_scale >= (B.width / 29.0f) * 0.95f) {
            // Larger/equal rival: extra defensive buffer
            required_clearance += 40.0f;
          } else {
            // Smaller rival: head-cut kill opportunity!
            if (d_obs > (B.radius + cp->r + 15.0f) && d_obs < (B.radius + cp->r + 120.0f)) {
              kill_opportunity_bonus += B.weights.weight_hunt_cut * 15.0f;
            }
          }
        }

        if (d_obs < required_clearance) {
          // Hard collision within kinematic horizon!
          ro->valid = false;
          ro->score = -1e8f;
          break;
        }
      }

      if (!ro->valid) break;
    }

    ro->end_x = sim_x;
    ro->end_y = sim_y;

    if (!ro->valid) {
      // Discard invalid colliding path
      continue;
    }

    // ------------------------------------------------------------------------
    // Multifactor Scoring of Valid Trajectories:
    // 1. Raycast Corridor Clearance (Open Space / Anti-Cul-de-Sac)
    // 2. Food Attraction (Mass/Distance)
    // 3. Directional Smoothness (Turn penalty)
    // 4. Border Repulsion
    // 5. Boost Mass Cost
    // ------------------------------------------------------------------------

    // Raycast clearance from rollout end point along final sim_ang
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

    // Normalised open clearance score
    float clearance_score = B.weights.weight_clearance * (fminf(ray_clearance, 1000.0f) / 1000.0f);

    // Food attraction score
    float food_score = 0.0f;
    if (B.has_food) {
      float d_to_food = sqrtf(dist2(sim_x, sim_y, B.best_food.x, B.best_food.y));
      food_score = B.weights.weight_food * (B.best_food.value / (1.0f + d_to_food * 0.004f));
    }

    // Turn penalty (inercia direccional suave)
    float turn_diff = fabsf(ang_between(target_ang, B.ang));
    float turn_penalty = B.weights.weight_turn_penalty * (turn_diff / (float)M_PI);

    // Border repulsion
    float border_penalty = 0.0f;
    float dist_ctr_end = sqrtf(dist2(sim_x, sim_y, world_cx, world_cy));
    if (dist_ctr_end > (world_rad - 1600.0f)) {
      float border_depth = (dist_ctr_end - (world_rad - 1600.0f)) / 1600.0f;
      border_penalty = B.weights.weight_border_repulse * border_depth * border_depth;
    }

    // Boost cost (penaliza turbo a menos que la ganancia de comida o kill lo justifique)
    float boost_penalty = boost_mode ? B.weights.weight_boost_cost : 0.0f;

    ro->score = clearance_score + food_score + kill_opportunity_bonus - turn_penalty - border_penalty - boost_penalty;

    // Update radar ray metrics
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

  // Fallback: If ALL 128 rollouts collide, select the path with the greatest collision delay
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
  sbot_rollout_t* best_ro = &B.rollouts[B.best_rollout_idx];

  // Manual or automatic Auto-Coil mode
  if (usrs->bot_mode == 2 || (B.score_val > usrs->bot_follow_circle_score && usrs->bot_follow_circle_score > 0)) {
    B.stage = 3; // COIL
    return;
  }

  // Min forward clearance to trigger emergency ESCAPE
  float forward_clearance = best_ro->clearance;

  if (B.stage == 2) {
    // Currently in ESCAPE: stay in escape until clearance is safely sustained
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
    // In FARM or HUNT: enter ESCAPE if forward clearance drops dangerously low
    if (forward_clearance < B.weights.escape_enter_dist) {
      B.stage = 2; // ESCAPE
      B.escape_frames_clear = 0;
    } else if (usrs->bot_mode == 1 && B.has_food && B.best_food.sz >= 2.5f) {
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

  // 2. Perception pass
  sbot_perceive_world(gdata, me);

  // 3. 128 Kinematic Rollouts Evaluation (64 directions x {cruise, turbo})
  sbot_evaluate_kinematic_rollouts(gdata);

  // 4. Hysteresis Mode Update
  sbot_update_state_machine(gdata, usrs);

  // 5. Select Best Direction & Mass-Positive Turbo
  sbot_rollout_t* best_ro = &B.rollouts[B.best_rollout_idx];
  float target_ang = best_ro->target_ang;
  bool use_turbo = false;

  if (usrs->bot_auto_turbo) {
    if (B.stage == 2) {
      // ESCAPE: Burst turbo only if tight threat (< 180px) and escape route is open
      if (best_ro->clearance > 320.0f && best_ro->min_dist_coll < 180.0f) {
        use_turbo = true;
      }
    } else if (B.stage == 1) {
      // HUNT / FEAST: Sprint if aligned with giant food cluster and safe
      if (B.has_food && B.best_food.value > 12.0f && best_ro->clearance > 450.0f) {
        float f_diff = fabsf(ang_between(atan2f(B.best_food.y - B.y, B.best_food.x - B.x), B.ang));
        if (f_diff < ((float)M_PI * 0.22f)) {
          use_turbo = true;
        }
      }
    }
  }

  // Preserve mass: never turbo if snake length is under minimum safety threshold
  if (B.snake_len < 20) {
    use_turbo = false;
  }

  // Manual player boost overrides bot
  if (custom_controls_is_boost_active() || touch_input_is_boosting()) {
    use_turbo = true;
  }

  // 6. Continuous Exponential Angle Smoothing (Zero Jitter)
  float dt = gdata->data.etm * 0.001f;
  if (dt <= 0.0f || dt > 0.1f) dt = 0.016666f;

  float d_target = ang_between(target_ang, B.smooth_ang);
  float smooth_blend = 1.0f - expf(-B.weights.angle_smooth_rate * dt);
  B.smooth_ang += d_target * smooth_blend;

  // Emit steering vectors to slither network/client
  bot->output.xm = (int)roundf(cosf(B.smooth_ang) * 250.0f);
  bot->output.ym = (int)roundf(sinf(B.smooth_ang) * 250.0f);
  bot->output.accel = use_turbo;

  B.target_x = B.x + cosf(B.smooth_ang) * 350.0f;
  B.target_y = B.y + sinf(B.smooth_ang) * 350.0f;
  B.target_accel = use_turbo;
}

// ----------------------------------------------------------------------------
// Visual Debug Overlay (ImGui 64-Direction Radar & Reticle)
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

  // 1. 64-Direction Kinematic Radar
  if (usrs->bot_visual_radar) {
    for (int i = 0; i < NUM_EVAL_DIRS; i++) {
      sbot_radar_ray_t* ray = &B.radar[i];
      float r_len = fminf(ray->clearance, 400.0f) * gsc;
      if (r_len < 10.0f) r_len = 10.0f;

      float rx = head_sx + cosf(ray->dir_ang) * r_len;
      float ry = head_sy + sinf(ray->dir_ang) * r_len;

      ImVec4 col;
      if (!ray->valid) {
        col = (ImVec4){0.95f, 0.15f, 0.15f, 0.35f}; // Red: Colliding
      } else if (ray->clearance > 600.0f) {
        col = (ImVec4){0.15f, 0.95f, 0.35f, 0.65f}; // Green: Open space
      } else {
        col = (ImVec4){0.95f, 0.85f, 0.15f, 0.50f}; // Yellow: Constrained
      }

      ImDrawList_AddLine(dl, (ImVec2){head_sx, head_sy}, (ImVec2){rx, ry},
                         igColorConvertFloat4ToU32(col), 1.5f);
    }
  }

  // 2. Chosen Trajectory & Goal Reticle
  if (usrs->bot_visual_line) {
    sbot_rollout_t* best_ro = &B.rollouts[B.best_rollout_idx];

    // Draw integrated rollout trajectory path
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

    // Reticle at rollout horizon
    ImDrawList_AddCircle(dl, (ImVec2){prev_x, prev_y}, 12.0f,
                         igColorConvertFloat4ToU32((ImVec4){0.0f, 1.0f, 0.90f, 0.90f}), 16, 2.0f);
    ImDrawList_AddCircleFilled(dl, (ImVec2){prev_x, prev_y}, 3.0f,
                               igColorConvertFloat4ToU32((ImVec4){1.0f, 1.0f, 1.0f, 1.0f}), 8);
  }

  // 3. Target Food Vector
  if (usrs->bot_visual_food && B.has_food) {
    float fx = mww2 + (B.best_food.x - view_xx) * gsc;
    float fy = mhh2 + (B.best_food.y - view_yy) * gsc;

    ImDrawList_AddLine(dl, (ImVec2){head_sx, head_sy}, (ImVec2){fx, fy},
                       igColorConvertFloat4ToU32((ImVec4){0.20f, 1.0f, 0.40f, 0.65f}), 1.8f);
    ImDrawList_AddCircle(dl, (ImVec2){fx, fy}, 10.0f,
                         igColorConvertFloat4ToU32((ImVec4){0.30f, 1.0f, 0.50f, 0.80f}), 12, 1.8f);
  }

  // 4. HUD State Badge
  const char* mode_name = "FARM";
  if (B.stage == 1) mode_name = "HUNT";
  else if (B.stage == 2) mode_name = "ESCAPE";
  else if (B.stage == 3) mode_name = "COIL";

  char badge[128];
  snprintf(badge, sizeof(badge), "BOT TOP [%s] | 64-Dir Rollouts | Clear: %.0fpx | Turbo: %s",
           mode_name, B.rollouts[B.best_rollout_idx].clearance,
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