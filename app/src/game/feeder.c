#include "feeder.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../external/mongoose.h"
#include "../network/callback.h"
#include "../user.h"
#include "game_data.h"
#include "snake.h"
#include "user_settings.h"
#include "../constants.h"

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "feeder_bot", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "feeder_bot", __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__)
#define LOGE(...) printf(__VA_ARGS__)
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

static inline float ang_between(float a, float b) {
  float d = fmodf(a - b, (float)PI2);
  if (d < -(float)M_PI) d += (float)PI2;
  if (d > (float)M_PI) d -= (float)PI2;
  return d;
}

typedef struct feeder_bot {
  int id;
  struct mg_connection* c;
  int snake_id;
  float x;
  float y;
  float ang;
  bool alive;
  bool connecting;
  bool boosted;
  double last_steer_time;
  double last_ping_time;
  double last_frame_time;
  double cooldown_until;
  char name[24];
} feeder_bot;

static struct mg_mgr s_feeder_mgr;
static bool s_feeder_mgr_inited = false;
static feeder_bot s_bots[MAX_FEEDER_BOTS];
static bool s_feeder_enabled = false;
static int s_target_count = 3; // Default to 3 concurrent feeder bots for continuous feeding chain
static float s_closest_dist = -1.0f;
static double s_last_bot_spawn = 0.0;

static void feeder_bot_on_packet(feeder_bot* bot, const uint8_t* pkt, int len, double now) {
  if (!bot || !pkt || len <= 0) return;
  uint8_t cmd = pkt[0];

  if (cmd == '6') {
    // Challenge packet: solve and send response + spawn packet
    uint8_t secret[27] = {0};
    decode_secret(pkt, len, secret);
    mg_ws_send(bot->c, secret, 27, WEBSOCKET_OP_BINARY);

    uint8_t ba[64];
    int nick_len = (int)strlen(bot->name);
    ba[0] = 115; // 's'
    ba[1] = 30;
    ba[2] = (291 >> 8) & 255;
    ba[3] = 291 & 255;
    uint8_t cwa[20] = {54, 206, 204, 169, 97, 178, 74,  136, 124, 117,
                       14, 210, 106, 236, 8,  208, 136, 213, 140, 111};
    memcpy(ba + 4, cwa, 20);
    ba[24] = 8; // Bright lime green skin for feeder bots
    ba[25] = (uint8_t)nick_len;
    memcpy(ba + 26, bot->name, nick_len);
    int bm = 26 + nick_len;
    ba[bm++] = 0; // accessory
    ba[bm++] = 0; // custom skin flag
    mg_ws_send(bot->c, ba, bm, WEBSOCKET_OP_BINARY);
    LOGI("feeder_bot [%s]: Sent challenge response & spawn request", bot->name);
  } else if (cmd == 's') {
    int dlen = len - 1;
    if (dlen > 6) {
      int s_id = (pkt[1] << 8) | pkt[2];
      bool is_our_bot = false;
      if (bot->snake_id == -1) {
        if (len >= 27) {
          int nl = pkt[22];
          if (nl > 0 && 23 + nl <= len) {
            if (strncmp((const char*)(pkt + 23), bot->name, strlen(bot->name)) == 0 ||
                strncmp((const char*)(pkt + 23), "[FEED]", 6) == 0) {
              is_our_bot = true;
            }
          }
        }
        if (!is_our_bot && bot->connecting) {
          is_our_bot = true;
        }
      }
      if (is_our_bot) {
        bot->snake_id = s_id;
        if (len >= 22) {
          float snx = (float)((pkt[16] << 16) | (pkt[17] << 8) | pkt[18]) / 5.0f;
          float sny = (float)((pkt[19] << 16) | (pkt[20] << 8) | pkt[21]) / 5.0f;
          bot->x = snx;
          bot->y = sny;
        }
        bot->alive = true;
        bot->connecting = false;
        bot->last_steer_time = 0;
        bot->last_ping_time = now;
        bot->last_frame_time = now;
        LOGI("feeder_bot [%s]: Spawned snake_id=%d at (%.1f, %.1f)", bot->name, s_id, bot->x, bot->y);
      }
    } else if (bot->alive && dlen >= 2) {
      int s_id = (pkt[1] << 8) | pkt[2];
      if (s_id == bot->snake_id) {
        LOGI("feeder_bot [%s]: Crashed into player snake body! Mass released. Mission accomplished!", bot->name);
        bot->alive = false;
        if (bot->c) {
          bot->c->is_closing = true;
          bot->c = NULL;
        }
        bot->snake_id = -1;
        bot->cooldown_until = now + 2.5; // Smooth 2.5s respawn cycle
      }
    }
  } else if (cmd == '=' && len == 7) {
    int m = 1;
    float iang = (float)((pkt[m] << 8) | pkt[m + 1]);
    m += 2;
    float xx = (float)((pkt[m] << 8) | pkt[m + 1]);
    m += 2;
    float yy = (float)((pkt[m] << 8) | pkt[m + 1]);
    bot->x = xx;
    bot->y = yy;
    bot->ang = iang * GD_K64A;
  } else if (cmd == '+' && len == 10) {
    int m = 1;
    float iang = (float)((pkt[m] << 8) | pkt[m + 1]);
    m += 2;
    float xx = (float)((pkt[m] << 8) | pkt[m + 1]);
    m += 2;
    float yy = (float)((pkt[m] << 8) | pkt[m + 1]);
    bot->x = xx;
    bot->y = yy;
    bot->ang = iang * GD_K64A;
  } else if (cmd == 'G' && len == 3) {
    float iang = (float)((pkt[1] << 8) | pkt[2]);
    bot->ang = iang * GD_K64A;
  } else if (cmd == 'N' && len == 6) {
    float iang = (float)((pkt[1] << 8) | pkt[2]);
    bot->ang = iang * GD_K64A;
  } else if (cmd == 'v') {
    if (bot->alive) {
      LOGI("feeder_bot [%s]: Death packet 'v' received", bot->name);
      bot->alive = false;
      if (bot->c) {
        bot->c->is_closing = true;
        bot->c = NULL;
      }
      bot->snake_id = -1;
      bot->cooldown_until = now + 2.5;
    }
  }
}

static void feeder_ws_cb(struct mg_connection* c, int ev, void* ev_data) {
  feeder_bot* bot = (feeder_bot*)c->fn_data;
  double now = get_monotonic_sec();

  if (ev == MG_EV_WS_OPEN) {
    LOGI("feeder_ws_cb [%s]: WebSocket handshake established! Sending init bytes", bot ? bot->name : "unknown");
    mg_ws_send(c, (uint8_t[]){1}, 1, WEBSOCKET_OP_BINARY);
    mg_ws_send(c, (uint8_t[]){'c', 0}, 2, WEBSOCKET_OP_BINARY);
    if (bot) bot->connecting = false;
  } else if (ev == MG_EV_WS_MSG) {
    struct mg_ws_message* msg = (struct mg_ws_message*)ev_data;
    uint8_t* a = (uint8_t*)msg->data.buf;
    int total_len = (int)msg->data.len;
    int m = 0;

    if (total_len > 0 && a[0] < 32) {
      while (m < total_len) {
        int sub_len;
        if (a[m] < 32) {
          if (m + 1 >= total_len) break;
          sub_len = (a[m] << 8) | a[m + 1];
          m += 2;
        } else {
          sub_len = a[m] - 32;
          m += 1;
        }
        if (m + sub_len > total_len) break;
        if (bot) feeder_bot_on_packet(bot, a + m, sub_len, now);
        m += sub_len;
      }
    } else if (bot && total_len > 0) {
      feeder_bot_on_packet(bot, a, total_len, now);
    }
  } else if (ev == MG_EV_ERROR) {
    LOGI("feeder_ws_cb [%s]: Connection error", bot ? bot->name : "unknown");
    c->is_closing = true;
    if (bot) {
      bot->alive = false;
      bot->connecting = false;
      bot->c = NULL;
      bot->snake_id = -1;
      bot->cooldown_until = now + 2.5;
    }
  } else if (ev == MG_EV_CLOSE) {
    if (bot) {
      bot->alive = false;
      bot->connecting = false;
      bot->c = NULL;
      bot->snake_id = -1;
      bot->cooldown_until = now + 2.5;
    }
  }
}

void feeder_init(tenv* env) {
  (void)env;
  if (!s_feeder_mgr_inited) {
    mg_mgr_init(&s_feeder_mgr);
    s_feeder_mgr_inited = true;
  }
  memset(s_bots, 0, sizeof(s_bots));
  for (int i = 0; i < MAX_FEEDER_BOTS; i++) {
    s_bots[i].id = i;
    s_bots[i].snake_id = -1;
    snprintf(s_bots[i].name, sizeof(s_bots[i].name), "[FEED] #%d", i + 1);
  }
  s_closest_dist = -1.0f;
  s_target_count = 3;
  s_last_bot_spawn = 0.0;
}

void feeder_update(tenv* env) {
  if (!s_feeder_mgr_inited) return;
  tuser_data* usr = env->usr;
  game_data* gdata = &usr->gdata;
  user_settings* usrs = &usr->usrs;
  double now = get_monotonic_sec();

  // Check if feeder should be active
  if (!s_feeder_enabled || gdata->conn != CONNECTED || gdata->data.dead) {
    for (int i = 0; i < MAX_FEEDER_BOTS; i++) {
      if (s_bots[i].c) {
        s_bots[i].c->is_closing = true;
        s_bots[i].c = NULL;
      }
      s_bots[i].alive = false;
      s_bots[i].connecting = false;
      s_bots[i].snake_id = -1;
      s_bots[i].cooldown_until = 0.0;
    }
    s_closest_dist = -1.0f;
    s_last_bot_spawn = 0.0;
    mg_mgr_poll(&s_feeder_mgr, 0);
    return;
  }

  snake* me = get_snake(gdata, gdata->data.snake_id);
  if (!me) {
    mg_mgr_poll(&s_feeder_mgr, 0);
    return;
  }

  int pts_len = tdarray_length(me->pts);

  int target_count = s_target_count;
  if (usrs && usrs->feeder_bot_count > 0) {
    target_count = usrs->feeder_bot_count;
  }
  if (target_count < 1) target_count = 1;
  if (target_count > MAX_FEEDER_BOTS) target_count = MAX_FEEDER_BOTS;
  s_target_count = target_count;

  float min_dist = 999999.0f;
  int alive_count = 0;

  for (int i = 0; i < MAX_FEEDER_BOTS; i++) {
    feeder_bot* bot = &s_bots[i];

    if (i >= target_count) {
      if (bot->c) {
        bot->c->is_closing = true;
        bot->c = NULL;
      }
      bot->alive = false;
      bot->connecting = false;
      bot->snake_id = -1;
      continue;
    }

    // Watchdog: recycle connecting bot if handshake times out
    if (bot->connecting && now - bot->last_frame_time > 7.0) {
      LOGI("feeder_bot [%s]: Handshake timeout, recycling socket...", bot->name);
      if (bot->c) {
        bot->c->is_closing = true;
        bot->c = NULL;
      }
      bot->connecting = false;
      bot->cooldown_until = now + 2.5;
    }

    // Spawn bot if disconnected and cooldown expired
    if (!bot->c && !bot->connecting && now >= bot->cooldown_until) {
      // Stagger bot connections by 2.8s to strictly respect server per-IP connection limits
      if (now - s_last_bot_spawn < 2.8) {
        continue;
      }
      s_last_bot_spawn = now;

      char url[256];
      snprintf(url, sizeof(url), "ws://%s/slither", usrs->ipv4);
      bot->id = i;
      snprintf(bot->name, sizeof(bot->name), "[FEED] #%d", i + 1);
      bot->snake_id = -1;
      bot->alive = false;
      bot->boosted = false;
      bot->last_steer_time = 0;
      bot->last_ping_time = now;
      bot->last_frame_time = now;
      bot->c = mg_ws_connect(&s_feeder_mgr, url, feeder_ws_cb, bot,
                             "%s:%s\r\n", "Origin", "https://slither.com");
      if (bot->c) {
        bot->connecting = true;
      } else {
        bot->cooldown_until = now + 2.5;
      }
      break; // Only spawn one bot per frame to stagger connections cleanly
    }

    // Steer, predict position, and boost active bot
    if (bot->alive && bot->c) {
      alive_count++;

      // Dead reckoning between server packets
      float dt = (float)(now - bot->last_frame_time);
      if (dt > 0.0f && dt < 0.25f) {
        float spd = bot->boosted ? 840.0f : 345.0f;
        bot->x += cosf(bot->ang) * spd * dt;
        bot->y += sinf(bot->ang) * spd * dt;
      }
      bot->last_frame_time = now;

      // CRITICAL: Calculate target point on player's BODY for THIS SPECIFIC BOT.
      // In Slither.io collision physics:
      // When a snake's head hits another snake's BODY, the colliding snake dies instantly,
      // and the snake whose body was hit takes 0 damage and survives!
      // By finding the closest body segment behind the neck, we ensure:
      // 1. Minimum transit distance into the body.
      // 2. Direct perpendicular T-bone collision into the body.
      // 3. Absolute prevention of head-to-head collisions.
      float target_x = me->xx;
      float target_y = me->yy;

      if (pts_len >= 6) {
        int safe_max = pts_len - 4; // At least 4 segments back from the head
        float best_d2 = 1e12f;
        int best_j = pts_len / 2;
        for (int j = 0; j <= safe_max; j++) {
          float bdx = bot->x - me->pts[j].xx;
          float bdy = bot->y - me->pts[j].yy;
          float d2 = bdx * bdx + bdy * bdy;
          if (d2 < best_d2) {
            best_d2 = d2;
            best_j = j;
          }
        }
        target_x = me->pts[best_j].xx;
        target_y = me->pts[best_j].yy;
      } else if (pts_len > 0) {
        // Small snake: target tail point (index 0) to ensure body contact without head-to-head collision
        target_x = me->pts[0].xx;
        target_y = me->pts[0].yy;
      } else {
        // Fallback: 150 units behind head orientation
        target_x = me->xx - cosf(me->ang) * 150.0f;
        target_y = me->yy - sinf(me->ang) * 150.0f;
      }

      // Head-Collision Prevention / Flanking Maneuver:
      // If the feeder bot is in front of the player snake's head (within forward 160 deg arc and < 1400u),
      // apply lateral offset away from the player's heading vector to curve smoothly around the head
      // and strike the body perpendicularly from the side!
      float to_bot_x = bot->x - me->xx;
      float to_bot_y = bot->y - me->yy;
      float dist_head = sqrtf(to_bot_x * to_bot_x + to_bot_y * to_bot_y);
      float ang_to_bot = atan2f(to_bot_y, to_bot_x);
      float diff = fabsf(ang_between(ang_to_bot, me->ang));
      if (diff < ((float)M_PI * 0.45f) && dist_head < 1400.0f) {
        float side_cross = cosf(me->ang) * to_bot_y - sinf(me->ang) * to_bot_x;
        float flank_sign = (side_cross >= 0.0f) ? 1.0f : -1.0f;
        float lateral_push = (1400.0f - dist_head) * 0.35f;
        target_x += -sinf(me->ang) * flank_sign * lateral_push;
        target_y += cosf(me->ang) * flank_sign * lateral_push;
      }

      float dx = target_x - bot->x;
      float dy = target_y - bot->y;
      float dist = sqrtf(dx * dx + dy * dy);
      if (dist < min_dist) min_dist = dist;

      // Steer every 40ms (25 Hz) towards player's body
      if (now - bot->last_steer_time > 0.04) {
        bot->last_steer_time = now;
        float target_ang = atan2f(dy, dx);
        target_ang = fmodf(target_ang, PI2);
        if (target_ang < 0) target_ang += PI2;
        int sang = (int)floorf(251.0f * target_ang / PI2);
        if (sang < 0) sang = 0;
        if (sang > 250) sang = 250;
        uint8_t pkt = (uint8_t)sang;
        mg_ws_send(bot->c, &pkt, 1, WEBSOCKET_OP_BINARY);
      }

      // Activate turbo boost when within 1100 units of player's body to smash into body at max velocity!
      // Also boost when far away (> 6500 units) to navigate to the player quickly across the map
      bool want_boost = (dist < 1100.0f || dist > 6500.0f);
      if (want_boost != bot->boosted) {
        bot->boosted = want_boost;
        uint8_t cmd = want_boost ? 253 : 254;
        mg_ws_send(bot->c, &cmd, 1, WEBSOCKET_OP_BINARY);
      }

      // Keepalive ping
      if (now - bot->last_ping_time > 1.2) {
        bot->last_ping_time = now;
        uint8_t ping_pkt = 251;
        mg_ws_send(bot->c, &ping_pkt, 1, WEBSOCKET_OP_BINARY);
      }
    }
  }

  if (alive_count > 0) {
    s_closest_dist = min_dist;
  } else {
    s_closest_dist = -1.0f;
  }

  mg_mgr_poll(&s_feeder_mgr, 0);
}

void feeder_destroy(tenv* env) {
  (void)env;
  if (s_feeder_mgr_inited) {
    for (int i = 0; i < MAX_FEEDER_BOTS; i++) {
      if (s_bots[i].c) {
        s_bots[i].c->is_closing = true;
        s_bots[i].c = NULL;
      }
      s_bots[i].alive = false;
      s_bots[i].connecting = false;
      s_bots[i].snake_id = -1;
    }
    mg_mgr_poll(&s_feeder_mgr, 0);
    mg_mgr_free(&s_feeder_mgr);
    s_feeder_mgr_inited = false;
  }
}

int feeder_get_active_count(void) {
  int count = 0;
  for (int i = 0; i < MAX_FEEDER_BOTS; i++) {
    if (s_bots[i].alive) count++;
  }
  return count;
}

int feeder_get_target_count(void) {
  return s_target_count;
}

void feeder_set_target_count(int count) {
  if (count < 1) count = 1;
  if (count > MAX_FEEDER_BOTS) count = MAX_FEEDER_BOTS;
  s_target_count = count;
}

bool feeder_is_enabled(void) {
  return s_feeder_enabled;
}

void feeder_set_enabled(bool enabled) {
  s_feeder_enabled = enabled;
  if (s_feeder_enabled) {
    s_last_bot_spawn = 0.0;
    for (int i = 0; i < MAX_FEEDER_BOTS; i++) {
      s_bots[i].cooldown_until = 0.0;
    }
  }
}

void feeder_toggle_enabled(void) {
  s_feeder_enabled = !s_feeder_enabled;
  if (s_feeder_enabled) {
    s_last_bot_spawn = 0.0;
    for (int i = 0; i < MAX_FEEDER_BOTS; i++) {
      s_bots[i].cooldown_until = 0.0;
    }
  }
}

float feeder_get_closest_dist(void) {
  return s_closest_dist;
}

int feeder_get_bots_pos(feeder_bot_pos* out_pos, int max_count) {
  if (!out_pos || max_count <= 0) return 0;
  int count = 0;
  for (int i = 0; i < MAX_FEEDER_BOTS && count < max_count; i++) {
    if (s_bots[i].alive) {
      out_pos[count].x = s_bots[i].x;
      out_pos[count].y = s_bots[i].y;
      out_pos[count].boosted = s_bots[i].boosted;
      out_pos[count].dist = 0.0f;
      out_pos[count].alive = true;
      count++;
    }
  }
  return count;
}
