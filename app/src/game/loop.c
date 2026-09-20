#include "loop.h"

#include "../network/server.h"
#include "../user.h"
#include "input.h"
#include "oef.h"
#include "redraw.h"
#include "ui_overlay.h"

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
      if (cur_t > TIMEOUT) {
        if (gdata->connection) gdata->connection->is_closing = true;
        LOGE("Connection timed out after %.2f seconds", cur_t);
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
        gdata->conn = DISCONNECTED;
        gdata->closed = false;
      }
      break;
    }
    case CONNECTED:
      time_step(env);
      input(env);
      server_poll(env);
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

      // Return to menu on death after brief explosion animation or on user tap
      if (gdata->data.dead && gdata->data.death_time > 0.0) {
        double dt = glfwGetTime() - gdata->data.death_time;
        bool user_tapped = tmouse_button_pressed(env->ms, GLFW_MOUSE_BUTTON_LEFT);
        if (dt >= 1.4 || (dt >= 0.25 && user_tapped)) {
          if (gdata->connection) {
            gdata->connection->is_closing = true;
            gdata->restart_req = usrs->instant_restart;
          }
        }
      }

      if (gdata->closed) {
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
