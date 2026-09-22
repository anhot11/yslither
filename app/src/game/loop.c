#include "loop.h"

#include "../network/server.h"
#include "../network/server_list.h"
#include "../user.h"
#include "input.h"
#include "oef.h"
#include "redraw.h"
#include "ui_overlay.h"
#include "flight_recorder.h"
#include "feeder.h"

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "yslither_loop", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "yslither_loop", __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__)
#define LOGE(...) printf(__VA_ARGS__)
#endif

void game_loop(tenv* env) {
  tuser_data* usr = env->usr;
  tcontext* ctx = env->ctx;
  game_data* gdata = &usr->gdata;
  user_settings* usrs = &usr->usrs;

  if (!env->config.running) gdata->conn = DISCONNECTED;

  switch (gdata->conn) {
    case CONNECTING: {
      usr->r->global.bg_opacity = 0;
      usr->r->global.bd_opacity = 0;
      usr->r->global.minimap_opacity = 0;

      double cur_t = glfwGetTime();
      if (cur_t > 3.5) { // 3.5 sec fast timeout per server attempt
        if (gdata->connection) gdata->connection->is_closing = true;
        LOGE("Connection timed out after %.2f seconds, triggering failover...", cur_t);
      }

      server_poll(env);

      vec2 loading_bar = {500, 12};
      igSetCursorPosX(ctx->size[0] * 0.5f - loading_bar[0] * 0.5f);
      igSetCursorPosY(ctx->size[1] * 0.5f - loading_bar[1] * 0.5f);

      igPushStyleColor_Vec4(ImGuiCol_PlotHistogram,
                            (ImVec4){0.168f, 0.668f, 0.375f, 1});
      igProgressBar(-glfwGetTime(), (ImVec2){loading_bar[0], loading_bar[1]},
                    NULL);
      igPopStyleColor(1);

      if (gdata->closed) {
        gdata->closed = false;
        // Smart Automatic Failover: Try up to 3 next best servers before giving up
        if (gdata->connect_retry_count < 3) {
          gdata->connect_retry_count++;
          const char* next_ip = server_list_get_fallback_ip(gdata->connect_retry_count);
          if (next_ip && next_ip[0] != '\0') {
            LOGI("Failover: Switching to alternative server %s (attempt %d/3)...", next_ip, gdata->connect_retry_count);
            strncpy(usrs->ipv4, next_ip, MAX_IPV4_LEN);
            glfwSetTime(0);
            server_connect(env);
            break;
          }
        }
        gdata->conn = DISCONNECTED;
        gdata->connect_retry_count = 0;
      }
      break;
    }
    case CONNECTED:
      gdata->connect_retry_count = 0;
      time_step(env);
      flight_recorder_record_frame(env);
      input(env);
      server_poll(env);
      feeder_update(env);
      oef(env);
      redraw(env);
      ui_overlay(env);

      // special hotkeys
      if (usrs->hotkeys[HOTKEY_QUIT].active ||
          (usrs->quit_mc &&
           tmouse_button_pressed(env->ms, GLFW_MOUSE_BUTTON_MIDDLE))) {
        gdata->connection->is_closing = true;
      } else if (usrs->hotkeys[HOTKEY_RESTART].active ||
                 (usrs->restart_rc &&
                   tmouse_button_pressed(env->ms, GLFW_MOUSE_BUTTON_RIGHT))) {
        gdata->connection->is_closing = true;
        gdata->restart_req = true;
      }

      // Death handling:
      // If Bot Mode is active OR user has instant_restart enabled:
      // automatically respawn in the match after death explosion!
      // Otherwise: keep in arena and let user tap "REAPARECER" or "SALIR" in the dialog!
      if (gdata->data.dead && gdata->data.death_time > 0.0) {
        flight_recorder_on_death(env);
        double dt = glfwGetTime() - gdata->data.death_time;
        if (usrs->instant_restart || usrs->hotkeys[HOTKEY_BOT].active) {
          if (dt >= 1.2) {
            if (gdata->connection) {
              gdata->connection->is_closing = true;
              gdata->restart_req = true;
            }
          }
        }
      }

      if (gdata->closed) {
        flight_recorder_init();
        game_data_reset(env);

        if (gdata->restart_req) {
          usr->gdata.conn = CONNECTING;
          glfwSetTime(0);
          server_connect(env);
          gdata->restart_req = false;
        } else {
          usr->gdata.conn = DISCONNECTED;
          gdata->curr_screen = TITLE_SCREEN;
        }
        gdata->closed = false;
      }

      break;
    case DISCONNECTED:
      usr->r->global.bg_opacity = 0;
      usr->r->global.bd_opacity = 0;
      usr->r->global.minimap_opacity = 0;

      gdata->curr_screen = TITLE_SCREEN;

      game_data_reset(env);
      server_poll(env);

      break;
  }
}
