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
static int s_target_count = 2; // Default to 2 concurrent feeder bots for maximum server stability
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
        bot->cooldown_until = now + 3.0; // Stagger next respawn cleanly
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
      bot->cooldown_until = now + 3.0;
    }
  }
}

static void feeder_ws_cb(struct mg_connection* c, int ev, void* ev_data) {
  feeder_bot* bot = (feeder_bot*)c->fn_data;
  double now = glfwGetTime();

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
      bot->cooldown_until = now + 3.0;
    }
  } else if (ev == MG_EV_CLOSE) {
    if (bot) {
      bot->alive = false;
      bot->connecting = false;
      bot->c = NULL;
      bot->snake_id = -1;
      bot->cooldown_until = now + 3.0;
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
  s_target_count = 2;
}

void feeder_update(tenv* env) {
  if (!s_feeder_mgr_inited) return;
  tuser_data* usr = env->usr;
  game_data* gdata = &usr->gdata;
  user_settings* usrs = &usr->usrs;
  double now = glfwGetTime();

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

  // CRITICAL REQUIREMENT: Feeder bots must steer directly into the player snake's BODY!
  // In Slither.io collision physics:
  // When a snake's head hits another snake's BODY, the colliding snake dies instantly,
  // and the snake whose body was hit takes 0 damage and survives!
  // By targeting the midpoint of the player's body (pts_len / 2),
  // we eliminate any risk of head-to-head collision.
  int pts_len = tdarray_length(me->pts);
  float target_x = me->xx;
  float target_y = me->yy;

  if (pts_len >= 12) {
    int mid_idx = pts_len / 2;
    target_x = me->pts[mid_idx].xx;
    target_y = me->pts[mid_idx].yy;
  } else if (pts_len >= 4) {
    int safe_idx = pts_len - 1 - 2;
    if (safe_idx < 0) safe_idx = 0;
    target_x = me->pts[safe_idx].xx;
    target_y = me->pts[safe_idx].yy;
  } else if (pts_len > 0) {
    // Small snake: target the tail (index 0) to guarantee body collision without risking head-to-head impact
    target_x = me->pts[0].xx;
    target_y = me->pts[0].yy;
  } else {
    // Fallback if no points yet: aim 120 units behind the current head heading
    target_x = me->xx - cosf(me->ang) * 120.0f;
    target_y = me->yy - sinf(me->ang) * 120.0f;
  }

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

    // Spawn bot if disconnected and cooldown expired
    if (!bot->c && !bot->connecting && now >= bot->cooldown_until) {
      // Stagger bot connections by 3.0s to strictly respect server per-IP connection limits
      if (now - s_last_bot_spawn < 3.0) {
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
        bot->cooldown_until = now + 3.0;
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

      float dx = target_x - bot->x;
      float dy = target_y - bot->y;
      float dist = sqrtf(dx * dx + dy * dy);
      if (dist < min_dist) min_dist = dist;

      // Steer every 50ms towards player's body
      if (now - bot->last_steer_time > 0.05) {
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

      // Activate turbo boost when within 2200 units of player's body to smash into body at max velocity!
      // Also boost when far away (> 6000 units) to navigate to the player quickly across the map
      bool want_boost = (dist < 2200.0f || dist > 6000.0f);
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
}

void feeder_toggle_enabled(void) {
  s_feeder_enabled = !s_feeder_enabled;
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
