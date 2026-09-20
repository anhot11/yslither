#include "ui_overlay.h"
#include "custom_controls.h"
#include "sbot.h"
#include "../user.h"
#include <stdio.h>
#include <math.h>

static void render_stats_hud(tenv* env) {
  if (!env || !env->usr || !env->ctx) return;
  tuser_data* usr = env->usr;
  game_data* gdata = &usr->gdata;

  // Only render during active gameplay
  if (tdarray_length(gdata->data.snakes) == 0) return;

  ImDrawList* fg_dl = igGetForegroundDrawList_ViewportPtr(igGetMainViewport());
  if (!fg_dl) return;

  // 1. Calculate live score / snake length (tamaño)
  int my_score = gdata->data.score;
  int ns = tdarray_length(gdata->data.snakes);
  for (int i = 0; i < ns; i++) {
    if (gdata->data.snakes[i].id == gdata->data.snake_id) {
      snake* me = &gdata->data.snakes[i];
      int sct = me->sct + me->rsc;
      if (sct >= 0 && gdata->data.fpsls && gdata->data.fmlts) {
        int calc_score = (int)floorf((gdata->data.fpsls[sct] + me->fam / gdata->data.fmlts[sct] - 1.0f) * 15.0f - 5.0f);
        if (calc_score > 0) my_score = calc_score;
      }
      if (my_score <= 0) {
        my_score = tdarray_length(me->pts) * 10;
      }
      break;
    }
  }
  if (my_score < 10) my_score = 10;

  // 2. Ping value (ms wifi)
  int ping_val = gdata->data.ping;
  if (ping_val <= 0 && gdata->data.last_ping_mtm > 0) {
    ping_val = (int)roundf(gdata->data.ctm - gdata->data.last_ping_mtm);
  }

  // Format labels
  char size_buf[48];
  snprintf(size_buf, sizeof(size_buf), "Tam: %d", my_score);

  char ping_buf[48];
  if (ping_val > 0) {
    snprintf(ping_buf, sizeof(ping_buf), "%d ms", ping_val);
  } else {
    snprintf(ping_buf, sizeof(ping_buf), "-- ms");
  }

  ImFont* font = usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR];
  if (!font) font = igGetFont();

  igPushFont(font, font->LegacySize);

  ImVec2 size_txt_sz;
  igCalcTextSize(&size_txt_sz, size_buf, NULL, false, -1);

  ImVec2 ping_txt_sz;
  igCalcTextSize(&ping_txt_sz, ping_buf, NULL, false, -1);

  float pad_x = 10.0f;
  float pad_y = 5.0f;
  float start_x = 16.0f;
  float start_y = 14.0f;

  // --- Badge 1: Snake Size (Tamaño) ---
  float card1_w = size_txt_sz.x + pad_x * 2.0f;
  float card_h = size_txt_sz.y + pad_y * 2.0f;

  ImDrawList_AddRectFilled(fg_dl, (ImVec2){start_x, start_y},
                           (ImVec2){start_x + card1_w, start_y + card_h},
                           igColorConvertFloat4ToU32((ImVec4){0.08f, 0.10f, 0.14f, 0.88f}), 8.0f, 0);
  ImDrawList_AddRect(fg_dl, (ImVec2){start_x, start_y},
                     (ImVec2){start_x + card1_w, start_y + card_h},
                     igColorConvertFloat4ToU32((ImVec4){0.95f, 0.75f, 0.20f, 0.75f}), 8.0f, 0, 1.5f);
  ImDrawList_AddText_Vec2(fg_dl, (ImVec2){start_x + pad_x, start_y + pad_y},
                          igColorConvertFloat4ToU32((ImVec4){0.98f, 0.86f, 0.28f, 1.0f}),
                          size_buf, NULL);

  // --- Badge 2: WiFi Ping ms ---
  float start2_x = start_x + card1_w + 10.0f;
  float card2_w = ping_txt_sz.x + pad_x * 2.0f;

  ImVec4 ping_color;
  if (ping_val <= 0) {
    ping_color = (ImVec4){0.70f, 0.75f, 0.80f, 1.0f};
  } else if (ping_val < 85) {
    ping_color = (ImVec4){0.20f, 0.90f, 0.45f, 1.0f}; // Verde brillante
  } else if (ping_val < 160) {
    ping_color = (ImVec4){0.95f, 0.82f, 0.25f, 1.0f}; // Amarillo
  } else {
    ping_color = (ImVec4){0.95f, 0.30f, 0.25f, 1.0f}; // Rojo
  }

  ImDrawList_AddRectFilled(fg_dl, (ImVec2){start2_x, start_y},
                           (ImVec2){start2_x + card2_w, start_y + card_h},
                           igColorConvertFloat4ToU32((ImVec4){0.08f, 0.10f, 0.14f, 0.88f}), 8.0f, 0);
  ImDrawList_AddRect(fg_dl, (ImVec2){start2_x, start_y},
                     (ImVec2){start2_x + card2_w, start_y + card_h},
                     igColorConvertFloat4ToU32((ImVec4){ping_color.x, ping_color.y, ping_color.z, 0.70f}), 8.0f, 0, 1.5f);
  ImDrawList_AddText_Vec2(fg_dl, (ImVec2){start2_x + pad_x, start_y + pad_y},
                          igColorConvertFloat4ToU32(ping_color),
                          ping_buf, NULL);

  igPopFont();
}

void ui_overlay(tenv* env) {
  tuser_data* usr = env->usr;
  tcontext* ctx = env->ctx;
  game_data* gdata = &usr->gdata;
  user_settings* usrs = &usr->usrs;

  float mww2 = ctx->size[0] / 2.0f;
  float mhh2 = ctx->size[1] / 2.0f;

  snake* me = get_snake(gdata, gdata->data.snake_id);
  if (!me && tdarray_length(gdata->data.snakes) > 0) {
    me = gdata->data.snakes + (tdarray_length(gdata->data.snakes) - 1);
  }
  if (me) {
      float a = me->alive_amt * (1 - me->dead_amt);
      int sct = me->sct + me->rsc;
      float hx = me->xx + me->fx;
      float hy = me->yy + me->fy;
      gdata->data.score = (int)floorf((gdata->data.fpsls[sct] +
                                       me->fam / gdata->data.fmlts[sct] - 1) *
                                          15 -
                                      5) /
                          1;

      if (usrs->hotkeys[HOTKEY_ASSIST].active) {
        ImDrawList_AddLine(
            igGetWindowDrawList(),
            (ImVec2){mww2 + (hx - gdata->data.view_xx) * gdata->data.gsc,
                     mhh2 + (hy - gdata->data.view_yy) * gdata->data.gsc},
            (ImVec2){env->ms->pos[0], env->ms->pos[1]},
            igColorConvertFloat4ToU32(
                (ImVec4){usrs->laser_color[0], usrs->laser_color[1],
                         usrs->laser_color[2], usrs->laser_color[3] * a}),
            usrs->laser_thickness);
      }
    }

  usr->r->global.minimap_opacity = 0;
  if (usrs->hotkeys[HOTKEY_HUD].active) {
    ImGuiStyle* style = igGetStyle();

    igPushFont(usr->imgui_data.mono_font[usrs->stats_font_size],
               usr->imgui_data.mono_font[usrs->stats_font_size]->LegacySize);
    float line_height = igGetCursorPosY();
    ImVec2 icon_sz;
    igCalcTextSize(&icon_sz, "\ue971", NULL, false, -1);
    ImVec2 char_sz;
    igCalcTextSize(&char_sz, "-", NULL, false, -1);
    igTextColored((ImVec4){1, 1, 1, 0.3}, "\ue971");
    igSameLine(0, -1);
    igTextColored((ImVec4){1, 1, 1, 0.5}, usrs->nickname);
    line_height = igGetCursorPosY() - line_height;

    igTextColored((ImVec4){1, 1, 1, 0.3}, "\ueaec");
    igSameLine(0, -1);
    igTextColored((ImVec4){1, 1, 1, 0.5}, usrs->ipv4);

    float ping_norm =
        (gdata->data.ping_follow - GOOD_PING) / (BAD_PING - GOOD_PING);
    float lag_norm = (gdata->data.lag_mult - 0.2f) / (1 - 0.2f);
    vec3 ping_col;
    glm_vec3_lerp((vec3){0.5f, 1, 0.5f}, (vec3){1, 0.5f, 0.5f}, ping_norm,
                  ping_col);
    vec3 ic_col;
    glm_vec3_lerp((vec3){1, 0.5f, 0.5f}, (vec3){1, 1, 1}, lag_norm, ic_col);

    igTextColored(
        (ImVec4){ic_col[0], ic_col[1], ic_col[2], glm_lerp(0.8, 0.3, lag_norm)},
        "\ue91b");
    igSameLine(0, -1);
    igTextColored(
        (ImVec4){ping_col[0], ping_col[1], ping_col[2], 0.6 * lag_norm},
        "%d ms", gdata->data.ping);

    igTextColored((ImVec4){1, 1, 1, 0.3}, "\ue99c");
    igSameLine(0, -1);
    igTextColored((ImVec4){1, 1, 1, 0.5}, "%d FPS", gdata->data.fps);

    igTextColored((ImVec4){1, 1, 1, 0.3}, "\ue952");
    igSameLine(0, -1);

    int tot_sec = (int)gdata->data.play_etm;
    int hours = tot_sec / 3600;
    int minutes = (tot_sec % 3600) / 60;
    int seconds = tot_sec % 60;

    igTextColored((ImVec4){1, 1, 1, 0.7}, "%02d:%02d:%02d", hours, minutes,
                  seconds);
    igText("");

#ifndef __ANDROID__
    if (usrs->hotkeys[HOTKEY_MENU].active) {
      display_hotkeys(usr, (icon_sz.x - char_sz.x) * 0.5f,
                      usrs->stats_font_size);
    }
#endif

    float px = (((gdata->data.view_xx - gdata->data.grd) * 2) /
                ((gdata->data.flux_grd) * 2));
    float py = (((gdata->data.view_yy - gdata->data.grd) * 2) /
                ((gdata->data.flux_grd) * 2));
    int pang = (int)roundf(glm_deg(atan2f(-py, px)));
    if (pang < 0) pang += 360;
    int dst = (int)roundf(sqrtf(px * px + py * py) * 100.0f);

    igSetCursorPosY(ctx->size[1] - (line_height * 3) - style->WindowPadding.y);

    igTextColored((ImVec4){1, 1, 1, 0.3}, "\ueaeb");
    igSameLine(0, -1);
    igTextColored((ImVec4){1, 1, 1, 0.7}, "%d", gdata->data.kills);

    igTextColored((ImVec4){1, 1, 1, 0.3}, "\ue9d9");
    igSameLine(0, -1);
    // igPushFont(
    //     usr->imgui_data.mono_font_bold[usrs->stats_font_size],
    //     usr->imgui_data.mono_font_bold[usrs->stats_font_size]->LegacySize);
    igTextColored((ImVec4){1, 1, 1, 0.7}, "%d", gdata->data.rank);
    // igPopFont();
    igSameLine(0, 0);
    igTextColored((ImVec4){1, 1, 1, 0.5}, " / %d", gdata->data.slither_count);

    igTextColored((ImVec4){1, 1, 1, 0.3}, "\ue99e");
    igSameLine(0, -1);
    igPushFont(
        usr->imgui_data.mono_font_bold[usrs->stats_font_size],
        usr->imgui_data.mono_font_bold[usrs->stats_font_size]->LegacySize);
    igTextColored((ImVec4){1, 1, 1, 0.7}, "%d", gdata->data.score);
    igPopFont();

    igPopFont();

    if (gdata->data.gotlb) {
      igPushFont(usr->imgui_data.mono_font[usrs->lb_font_size],
                 usr->imgui_data.mono_font[usrs->lb_font_size]->LegacySize);
      ImVec2 psize;
      igCalcTextSize(&psize, "10.", NULL, false, -1);

      ImVec2 nksize;
      char tmp[MAX_NICKNAME_LEN + 1] = {0};
      memset(tmp, (int)'a', MAX_NICKNAME_LEN);
      igCalcTextSize(&nksize, tmp, NULL, false, -1);
      nksize.x *= 1.25f;

      ImVec2 scsize;
      igCalcTextSize(&scsize, "999999", NULL, false, -1);
      igPopFont();

      float tb_width =
          psize.x + nksize.x + scsize.x + (style->CellPadding.x * 2 * 3);
      igSetCursorPosX(ctx->size[0] - tb_width - style->WindowPadding.x);
      igSetCursorPosY(style->WindowPadding.y);

      if (igBeginTable("leaderboard_table", 3, ImGuiTableFlags_NoHostExtendX,
                       (ImVec2){}, 0)) {
        igTableSetupColumn("##position", ImGuiTableColumnFlags_WidthFixed,
                           psize.x, 0);
        igTableSetupColumn("##nickname", ImGuiTableColumnFlags_WidthFixed,
                           nksize.x, 0);
        igTableSetupColumn("##score", ImGuiTableColumnFlags_WidthFixed,
                           scsize.x, 0);

        for (int row = 0; row < NUM_LEADERBOARD_ENTRIES; row++) {
          bool is_my_snake = gdata->data.lb_pos == (row + 1);
          vec3s* scolor = gdata->cg_colors + gdata->data.lb.entries[row].cv;
          vec3 tcolor;
          glm_vec3_lerp((float*)scolor, (vec3){1, 1, 1}, 0.4f, tcolor);
          ImVec4 itcolor = {tcolor[0], tcolor[1], tcolor[2], 1};
          if (is_my_snake) {
            igPushFont(
                usr->imgui_data.mono_font_bold[usrs->lb_font_size],
                usr->imgui_data.mono_font_bold[usrs->lb_font_size]->LegacySize);
          } else {
            igPushFont(
                usr->imgui_data.mono_font[usrs->lb_font_size],
                usr->imgui_data.mono_font[usrs->lb_font_size]->LegacySize);
            itcolor.w = 0.6f;  // .7f * (.3f + .7f * (1 - (1 + row) / 10.0f));
          }

          igTableNextRow(ImGuiTableRowFlags_None, 0);
          igTableSetColumnIndex(0);
          igTextColored((ImVec4){1, 1, 1, itcolor.w}, "%2d.", row + 1);
          igTableSetColumnIndex(1);
          igTextColored(itcolor, "%s", gdata->data.lb.entries[row].nickname);
          igTableSetColumnIndex(2);
          igTextColored(itcolor, "%d", gdata->data.lb.entries[row].score);

          igPopFont();
        }
        igEndTable();
      }
    }

    usr->r->global.minimap_circ[2] = usrs->minimap_size;
    usr->r->global.minimap_circ[0] =
        ctx->size[0] - usr->r->global.minimap_circ[2] - style->WindowPadding.x;
    usr->r->global.minimap_circ[1] = ctx->size[1] -
                                     usr->r->global.minimap_circ[2] -
                                     style->WindowPadding.y - line_height;
    usr->r->global.minimap_opacity = 1;

    igPushFont(usr->imgui_data.mono_font[usrs->stats_font_size],
               usr->imgui_data.mono_font[usrs->stats_font_size]->LegacySize);
    ImVec2 lctxtsz;
    igCalcTextSize(&lctxtsz, "--360° 100%", NULL, false, -1);

    igSetCursorPosX(
        ctx->size[0] -
        (usr->r->global.minimap_circ[2] * 0.5 + style->WindowPadding.x) -
        lctxtsz.x * 0.5f);
    igSetCursorPosY(ctx->size[1] - style->WindowPadding.y - line_height);

    igTextColored((ImVec4){1, 1, 1, 0.3f}, "\ue947");
    igSameLine(0, -1);
    igTextColored((ImVec4){1, 1, 1, 0.7f}, "%d° %d%%", pang, dst);
    igPopFont();
  }
 
  // Render live in-game stats HUD: Snake Size (Tamaño) & WiFi Ping (ms)
  render_stats_hud(env);

  // Render visual debug overlays for Bot Mode (line, red zones, sensors, food target)
  sbot_render_overlay(env);

  // Render on-screen custom touch action buttons
  custom_controls_render_hud(env, (float)ctx->size[0], (float)ctx->size[1]);

  // Game Over overlay banner when dying
  if (gdata->data.dead && gdata->data.death_time > 0.0) {
    float card_w = fminf(460.0f, (float)ctx->size[0] * 0.85f);
    float card_h = 160.0f;
    float card_x = (ctx->size[0] - card_w) * 0.5f;
    float card_y = (ctx->size[1] - card_h) * 0.35f;

    ImDrawList* fg_dl = igGetForegroundDrawList_ViewportPtr(igGetMainViewport());
    if (fg_dl) {
      double dt = glfwGetTime() - gdata->data.death_time;
      float alpha = fminf(0.60f, (float)dt * 0.6f);
      ImDrawList_AddRectFilled(fg_dl, (ImVec2){0, 0}, (ImVec2){(float)ctx->size[0], (float)ctx->size[1]},
                               igColorConvertFloat4ToU32((ImVec4){0.04f, 0.05f, 0.08f, alpha}), 0, 0);

      ImDrawList_AddRectFilled(fg_dl, (ImVec2){card_x, card_y}, (ImVec2){card_x + card_w, card_y + card_h},
                               igColorConvertFloat4ToU32((ImVec4){0.11f, 0.13f, 0.18f, 0.95f}), 16.0f, 0);
      ImDrawList_AddRect(fg_dl, (ImVec2){card_x, card_y}, (ImVec2){card_x + card_w, card_y + card_h},
                         igColorConvertFloat4ToU32((ImVec4){0.85f, 0.22f, 0.28f, 0.90f}), 16.0f, 0, 2.5f);
    }

    igSetNextWindowPos((ImVec2){card_x, card_y}, ImGuiCond_Always, (ImVec2){0, 0});
    igSetNextWindowSize((ImVec2){card_w, card_h}, ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoBackground;

    if (igBegin("##death_banner", NULL, flags)) {
      igSpacing();
      igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_LARGE],
                 usr->imgui_data.regular_font_bold[FONT_SIZE_LARGE]->LegacySize);
      ImVec2 title_sz;
      igCalcTextSize(&title_sz, "\ueaeb  ¡HAS MUERTO!", NULL, false, -1);
      igSetCursorPosX((card_w - title_sz.x) * 0.5f);
      igTextColored((ImVec4){0.95f, 0.28f, 0.32f, 1.0f}, "\ueaeb  ¡HAS MUERTO!");
      igPopFont();

      igSpacing();

      char stats_buf[128];
      snprintf(stats_buf, sizeof(stats_buf), "Puntuacion: %d   |   Kills: %d",
               gdata->data.score, gdata->data.kills);
      ImVec2 st_sz;
      igCalcTextSize(&st_sz, stats_buf, NULL, false, -1);
      igSetCursorPosX((card_w - st_sz.x) * 0.5f);
      igTextColored((ImVec4){0.88f, 0.90f, 0.94f, 1.0f}, "%s", stats_buf);

      igSpacing();
      const char* return_msg = "Regresando al menu principal...";
      ImVec2 ret_sz;
      igCalcTextSize(&ret_sz, return_msg, NULL, false, -1);
      igSetCursorPosX((card_w - ret_sz.x) * 0.5f);
      igTextColored((ImVec4){0.55f, 0.65f, 0.75f, 0.85f}, "%s", return_msg);
    }
    igEnd();
  }
}