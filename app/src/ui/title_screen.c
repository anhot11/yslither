#include "title_screen.h"

#include <stdio.h>
#include <string.h>

#include "../network/server.h"
#include "../network/server_list.h"
#include "../user.h"

static bool s_show_server_selector = false;
static bool s_show_vk = false;
static char s_vk_buffer[MAX_NICKNAME_LEN + 1] = {0};

void ui_title_screen_init(tenv* env) {
  (void)env;
  server_list_init();
}

static void ui_virtual_keyboard(tenv* env) {
  if (!s_show_vk) return;

  tuser_data* usr = env->usr;
  tcontext* ctx = env->ctx;
  user_settings* usrs = &usr->usrs;
  ImGuiStyle* style = igGetStyle();

  ImVec2 modal_sz = {fminf(660.0f, ctx->size[0] * 0.95f), fminf(420.0f, ctx->size[1] * 0.90f)};
  igSetNextWindowSize(modal_sz, ImGuiCond_Always);
  igSetNextWindowPos((ImVec2){(ctx->size[0] - modal_sz.x) * 0.5f,
                             (ctx->size[1] - modal_sz.y) * 0.5f},
                     ImGuiCond_Always, (ImVec2){});

  igPushStyleColor_Vec4(ImGuiCol_WindowBg, (ImVec4){0.10f, 0.12f, 0.16f, 0.98f});
  igPushStyleColor_Vec4(ImGuiCol_Border, (ImVec4){0.20f, 0.75f, 0.45f, 0.70f});
  igPushStyleVar_Float(ImGuiStyleVar_WindowRounding, 16.0f);
  igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 2.0f);

  if (igBegin("##vk_modal", NULL,
              ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
    igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_LARGE],
               usr->imgui_data.regular_font_bold[FONT_SIZE_LARGE]->LegacySize);
    igTextColored((ImVec4){0.20f, 0.90f, 0.50f, 1.0f}, "Escribe tu Nickname");
    igPopFont();

    igSeparator();
    igSpacing();

    // Display current buffer box
    igPushFont(usr->imgui_data.mono_font_bold[FONT_SIZE_LARGE],
               usr->imgui_data.mono_font_bold[FONT_SIZE_LARGE]->LegacySize);
    char display_buf[MAX_NICKNAME_LEN + 4];
    snprintf(display_buf, sizeof(display_buf), "> %s_", s_vk_buffer);
    igPushStyleColor_Vec4(ImGuiCol_ChildBg, (ImVec4){0.05f, 0.07f, 0.10f, 1.0f});
    if (igBeginChild_Str("##name_box", (ImVec2){-1, 48}, true, ImGuiWindowFlags_None)) {
      igSetCursorPosY(10);
      igTextColored((ImVec4){0.95f, 0.95f, 0.95f, 1.0f}, "%s", display_buf);
    }
    igEndChild();
    igPopStyleColor(1);
    igPopFont();

    igSpacing();

    igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
               usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
    igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 8.0f);

    float btn_w = 46.0f;
    float btn_h = 44.0f;

    // Numbers row
    const char* row0 = "1234567890";
    float row0_start_x = (modal_sz.x - (10 * (btn_w + style->ItemSpacing.x))) * 0.5f;
    igSetCursorPosX(row0_start_x);
    for (int i = 0; i < 10; i++) {
      char label[2] = {row0[i], '\0'};
      if (igButton(label, (ImVec2){btn_w, btn_h})) {
        size_t len = strlen(s_vk_buffer);
        if (len < MAX_NICKNAME_LEN) {
          s_vk_buffer[len] = row0[i];
          s_vk_buffer[len + 1] = '\0';
        }
      }
      if (i < 9) igSameLine(0, -1);
    }

    // Row 1: QWERTYUIOP
    const char* row1 = "QWERTYUIOP";
    igSetCursorPosX(row0_start_x);
    for (int i = 0; i < 10; i++) {
      char label[2] = {row1[i], '\0'};
      if (igButton(label, (ImVec2){btn_w, btn_h})) {
        size_t len = strlen(s_vk_buffer);
        if (len < MAX_NICKNAME_LEN) {
          s_vk_buffer[len] = row1[i];
          s_vk_buffer[len + 1] = '\0';
        }
      }
      if (i < 9) igSameLine(0, -1);
    }

    // Row 2: ASDFGHJKL
    const char* row2 = "ASDFGHJKL";
    float row2_start_x = (modal_sz.x - (9 * (btn_w + style->ItemSpacing.x))) * 0.5f;
    igSetCursorPosX(row2_start_x);
    for (int i = 0; i < 9; i++) {
      char label[2] = {row2[i], '\0'};
      if (igButton(label, (ImVec2){btn_w, btn_h})) {
        size_t len = strlen(s_vk_buffer);
        if (len < MAX_NICKNAME_LEN) {
          s_vk_buffer[len] = row2[i];
          s_vk_buffer[len + 1] = '\0';
        }
      }
      if (i < 8) igSameLine(0, -1);
    }

    // Row 3: ZXCVBNM + Backspace
    const char* row3 = "ZXCVBNM";
    float row3_start_x = (modal_sz.x - (7 * (btn_w + style->ItemSpacing.x) + 84.0f + style->ItemSpacing.x)) * 0.5f;
    igSetCursorPosX(row3_start_x);
    for (int i = 0; i < 7; i++) {
      char label[2] = {row3[i], '\0'};
      if (igButton(label, (ImVec2){btn_w, btn_h})) {
        size_t len = strlen(s_vk_buffer);
        if (len < MAX_NICKNAME_LEN) {
          s_vk_buffer[len] = row3[i];
          s_vk_buffer[len + 1] = '\0';
        }
      }
      igSameLine(0, -1);
    }
    // Backspace button
    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.60f, 0.20f, 0.20f, 1.0f});
    if (igButton("Borrar", (ImVec2){84.0f, btn_h})) {
      size_t len = strlen(s_vk_buffer);
      if (len > 0) {
        s_vk_buffer[len - 1] = '\0';
      }
    }
    igPopStyleColor(1);

    igSpacing();

    // Row 4: Space, Save (Listo), Cancel
    float bottom_row_w = 140.0f + 180.0f + 120.0f + 2 * style->ItemSpacing.x;
    igSetCursorPosX((modal_sz.x - bottom_row_w) * 0.5f);

    if (igButton("Espacio", (ImVec2){140.0f, 48.0f})) {
      size_t len = strlen(s_vk_buffer);
      if (len < MAX_NICKNAME_LEN && len > 0 && s_vk_buffer[len - 1] != ' ') {
        s_vk_buffer[len] = ' ';
        s_vk_buffer[len + 1] = '\0';
      }
    }
    igSameLine(0, -1);

    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.15f, 0.72f, 0.35f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.20f, 0.85f, 0.42f, 1.0f});
    if (igButton("Guardar", (ImVec2){180.0f, 48.0f})) {
      strncpy(usrs->nickname, s_vk_buffer, MAX_NICKNAME_LEN);
      save_user_settings(usrs);
      s_show_vk = false;
    }
    igPopStyleColor(2);
    igSameLine(0, -1);

    if (igButton("Cancelar", (ImVec2){120.0f, 48.0f})) {
      s_show_vk = false;
    }

    igPopStyleVar(1);
    igPopFont();
  }
  igEnd();

  igPopStyleVar(2);
  igPopStyleColor(2);
}

static void ui_server_selector(tenv* env) {
  if (!s_show_server_selector) return;

  tuser_data* usr = env->usr;
  tcontext* ctx = env->ctx;
  user_settings* usrs = &usr->usrs;
  ImGuiStyle* style = igGetStyle();

  ImVec2 modal_sz = {fminf(680.0f, ctx->size[0] * 0.95f), fminf(580.0f, ctx->size[1] * 0.92f)};
  igSetNextWindowSize(modal_sz, ImGuiCond_Always);
  igSetNextWindowPos((ImVec2){(ctx->size[0] - modal_sz.x) * 0.5f,
                             (ctx->size[1] - modal_sz.y) * 0.5f},
                     ImGuiCond_Always, (ImVec2){});

  igPushStyleColor_Vec4(ImGuiCol_WindowBg, (ImVec4){0.10f, 0.12f, 0.16f, 0.98f});
  igPushStyleColor_Vec4(ImGuiCol_Border, (ImVec4){0.25f, 0.55f, 0.90f, 0.70f});
  igPushStyleVar_Float(ImGuiStyleVar_WindowRounding, 16.0f);
  igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 2.0f);

  if (igBegin("##server_modal", NULL,
              ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
    igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_LARGE],
               usr->imgui_data.regular_font_bold[FONT_SIZE_LARGE]->LegacySize);
    igTextColored((ImVec4){0.35f, 0.70f, 1.0f, 1.0f}, "Seleccionar Servidor Slither.io");
    igPopFont();

    igTextColored((ImVec4){0.70f, 0.75f, 0.80f, 1.0f},
                  "Servidores oficiales de alta velocidad con medidor de ping en tiempo real:");
    igSeparator();
    igSpacing();

    igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
               usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
    igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 10.0f);

    // Auto best ping button
    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.15f, 0.65f, 0.35f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.20f, 0.80f, 0.42f, 1.0f});
    if (igButton("Mejor Ping Automatico", (ImVec2){modal_sz.x * 0.58f, 46.0f})) {
      const char* best_ip = server_list_get_best_ip();
      if (best_ip && best_ip[0] != '\0') {
        strncpy(usrs->ipv4, best_ip, MAX_IPV4_LEN);
        save_user_settings(usrs);
        s_show_server_selector = false;
      }
    }
    igPopStyleColor(2);
    igSameLine(0, -1);

    if (igButton("Actualizar Pings", (ImVec2){-1, 46.0f})) {
      server_list_refresh_pings();
    }
    igPopStyleVar(1);
    igPopFont();

    igSpacing();

    // Server list scrollable container
    float list_h = modal_sz.y - 180.0f;
    if (igBeginChild_Str("##srv_list", (ImVec2){-1, list_h}, true, ImGuiWindowFlags_None)) {
      int count = server_list_count();
      for (int i = 0; i < count; i++) {
        server_entry* s = server_list_get(i);
        if (!s) continue;

        char srv_addr[64];
        snprintf(srv_addr, sizeof(srv_addr), "%s:%d", s->ip, s->port);
        bool is_selected = (strcmp(usrs->ipv4, srv_addr) == 0);

        igPushID_Int(i);
        if (is_selected) {
          igPushStyleColor_Vec4(ImGuiCol_ChildBg, (ImVec4){0.15f, 0.28f, 0.40f, 0.80f});
        } else {
          igPushStyleColor_Vec4(ImGuiCol_ChildBg, (ImVec4){0.12f, 0.14f, 0.18f, 0.60f});
        }
        igPushStyleVar_Float(ImGuiStyleVar_ChildRounding, 8.0f);

        char child_id[32];
        snprintf(child_id, sizeof(child_id), "srv_card_%d", i);
        if (igBeginChild_Str(child_id, (ImVec2){-1, 54}, true, ImGuiWindowFlags_None)) {
          // Ping badge
          igSetCursorPos((ImVec2){10, 14});
          if (s->ping_ms < 0) {
            igTextColored((ImVec4){0.35f, 0.70f, 1.0f, 1.0f}, "[Midiendo...]");
          } else if (s->ping_ms < 100) {
            igTextColored((ImVec4){0.20f, 0.90f, 0.40f, 1.0f}, "[%3d ms]", s->ping_ms);
          } else if (s->ping_ms < 180) {
            igTextColored((ImVec4){0.95f, 0.85f, 0.20f, 1.0f}, "[%3d ms]", s->ping_ms);
          } else if (s->ping_ms < 999) {
            igTextColored((ImVec4){0.95f, 0.45f, 0.20f, 1.0f}, "[%3d ms]", s->ping_ms);
          } else {
            igTextColored((ImVec4){0.85f, 0.25f, 0.25f, 1.0f}, "[Timeout]");
          }

          // Region & address
          igSameLine(130, -1);
          igSetCursorPosY(14);
          igTextColored((ImVec4){1.0f, 1.0f, 1.0f, 1.0f}, "%s", s->name);
          igSameLine(360, -1);
          igSetCursorPosY(14);
          igTextColored((ImVec4){0.60f, 0.65f, 0.70f, 1.0f}, "%s", srv_addr);

          // Select button
          igSameLine(-1, -1);
          igSetCursorPos((ImVec2){modal_sz.x - 170.0f, 8.0f});
          igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 6.0f);
          if (is_selected) {
            igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.10f, 0.45f, 0.25f, 1.0f});
            igButton("Activo", (ImVec2){100, 36});
            igPopStyleColor(1);
          } else {
            if (igButton("Elegir", (ImVec2){100, 36})) {
              strncpy(usrs->ipv4, srv_addr, MAX_IPV4_LEN);
              save_user_settings(usrs);
              s_show_server_selector = false;
            }
          }
          igPopStyleVar(1);
        }
        igEndChild();
        igPopStyleVar(1);
        igPopStyleColor(1);
        igPopID();
      }
    }
    igEndChild();

    igSpacing();
    igSetCursorPosX((modal_sz.x - 160.0f) * 0.5f);
    igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 8.0f);
    if (igButton("Cerrar", (ImVec2){160.0f, 44.0f})) {
      s_show_server_selector = false;
    }
    igPopStyleVar(1);
  }
  igEnd();

  igPopStyleVar(2);
  igPopStyleColor(2);
}

void ui_title_screen(tenv* env) {
  tuser_data* usr = env->usr;
  tcontext* ctx = env->ctx;
  user_settings* usrs = &usr->usrs;
  ImGuiStyle* style = igGetStyle();
  ImGuiIO* io = igGetIO_Nil();
  game_data* gdata = &usr->gdata;

  // Initialize server list on first frame
  server_list_init();

  // Version banner in top-right
  char version_str[32] = {0};
  snprintf(version_str, sizeof(version_str), "v%s", APP_VERSION);
  ImVec2 vtxtsz;
  igCalcTextSize(&vtxtsz, version_str, NULL, false, -1);
  igSetCursorPosX(ctx->size[0] - vtxtsz.x - 24.0f);
  igSetCursorPosY(16.0f);
  igPushFont(usr->imgui_data.regular_font[FONT_SIZE_REGULAR],
             usr->imgui_data.regular_font[FONT_SIZE_REGULAR]->LegacySize);
  igTextColored((ImVec4){0.20f, 0.85f, 0.45f, 1.0f}, "%s", version_str);
  igPopFont();

  usr->r->global.bg_opacity = 0;
  usr->r->global.bd_opacity = 0;
  usr->r->global.minimap_opacity = 0;

  // Center menu container width
  float menu_w = fminf(520.0f, ctx->size[0] * 0.50f);
  float center_x = (ctx->size[0] - menu_w) * 0.5f;

  // Top header: YSLITHER title & subtitle
  float start_y = fmaxf(24.0f, ctx->size[1] * 0.12f);
  igSetCursorPosX(center_x);
  igSetCursorPosY(start_y);

  igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_LARGE],
             usr->imgui_data.regular_font_bold[FONT_SIZE_LARGE]->LegacySize);
  ImVec2 title_sz;
  igCalcTextSize(&title_sz, "Y S L I T H E R", NULL, false, -1);
  igSetCursorPosX((ctx->size[0] - title_sz.x) * 0.5f);
  igTextColored((ImVec4){0.20f, 0.95f, 0.50f, 1.0f}, "Y S L I T H E R");
  igPopFont();

  igPushFont(usr->imgui_data.regular_font[FONT_SIZE_SMALL],
             usr->imgui_data.regular_font[FONT_SIZE_SMALL]->LegacySize);
  ImVec2 sub_sz;
  igCalcTextSize(&sub_sz, "Cliente Vulkan con Bot Defensivo de Supervivencia", NULL, false, -1);
  igSetCursorPosX((ctx->size[0] - sub_sz.x) * 0.5f);
  igTextColored((ImVec4){0.65f, 0.70f, 0.75f, 0.85f},
                "Cliente Vulkan con Bot Defensivo de Supervivencia");
  igPopFont();

  // Stats row (Record, Kills, Play time)
  int tot_sec = (int)usrs->play_time;
  int hours = tot_sec / 3600;
  int minutes = (tot_sec % 3600) / 60;
  int seconds = tot_sec % 60;

  char stats_str[96];
  snprintf(stats_str, sizeof(stats_str), "Record: %d   |   Kills: %d   |   Tiempo: %02d:%02d:%02d",
           usrs->score, usrs->kills, hours, minutes, seconds);
  ImVec2 stats_sz;
  igCalcTextSize(&stats_sz, stats_str, NULL, false, -1);
  igSetCursorPosX((ctx->size[0] - stats_sz.x) * 0.5f);
  igSpacing();
  igTextColored((ImVec4){0.80f, 0.85f, 0.90f, 0.70f}, "%s", stats_str);

  igSpacing();
  igSpacing();

  // Style rounding for modern touch controls
  igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 12.0f);
  igPushStyleVar_Float(ImGuiStyleVar_ItemSpacing, 12.0f);

  // 1. Nickname Field with Touch Editor Button
  igSetCursorPosX(center_x);
  float nick_btn_w = 120.0f;
  float nick_input_w = menu_w - nick_btn_w - style->ItemSpacing.x;

  igPushItemWidth(nick_input_w);
  igPushStyleColor_Vec4(ImGuiCol_FrameBg, (ImVec4){0.15f, 0.17f, 0.22f, 1.0f});
  igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
             usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);

  char nick_display[MAX_NICKNAME_LEN + 16];
  snprintf(nick_display, sizeof(nick_display), "Nombre: %s",
           usrs->nickname[0] != '\0' ? usrs->nickname : "(Sin nombre)");
  if (igButton(nick_display, (ImVec2){nick_input_w, 54.0f})) {
    strncpy(s_vk_buffer, usrs->nickname, MAX_NICKNAME_LEN);
    s_show_vk = true;
  }
  igSameLine(0, -1);

  igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.22f, 0.35f, 0.50f, 1.0f});
  igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.30f, 0.45f, 0.65f, 1.0f});
  if (igButton("Cambiar", (ImVec2){nick_btn_w, 54.0f})) {
    strncpy(s_vk_buffer, usrs->nickname, MAX_NICKNAME_LEN);
    s_show_vk = true;
  }
  igPopStyleColor(3);
  igPopItemWidth();
  igPopFont();

  // 2. Server Selector Button
  igSetCursorPosX(center_x);
  const char* srv_name = server_list_get_name_by_ip(usrs->ipv4);
  int cur_ping = server_list_get_ping_by_ip(usrs->ipv4);

  char srv_btn_label[128];
  if (cur_ping > 0 && cur_ping < 999) {
    snprintf(srv_btn_label, sizeof(srv_btn_label), "Servidor: %s (%d ms)  ▼", srv_name, cur_ping);
  } else {
    snprintf(srv_btn_label, sizeof(srv_btn_label), "Servidor: %s  ▼", srv_name);
  }

  igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
             usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
  igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.16f, 0.20f, 0.28f, 1.0f});
  igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.22f, 0.28f, 0.38f, 1.0f});
  if (igButton(srv_btn_label, (ImVec2){menu_w, 54.0f})) {
    s_show_server_selector = true;
  }
  igPopStyleColor(2);
  igPopFont();

  igSpacing();

  // 3. JUGAR / PLAY Button (Giant primary CTA button)
  igSetCursorPosX(center_x);
  igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_LARGE],
             usr->imgui_data.regular_font_bold[FONT_SIZE_LARGE]->LegacySize);
  igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.14f, 0.72f, 0.35f, 1.0f});
  igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.18f, 0.85f, 0.42f, 1.0f});
  igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.10f, 0.60f, 0.28f, 1.0f});

  if (igButton("\uea1c  J U G A R", (ImVec2){menu_w, 64.0f})) {
    usr->gdata.conn = CONNECTING;
    usr->gdata.curr_screen = PLAYING;
    glfwSetTime(0);
    server_connect(env);
  }
  igPopStyleColor(3);
  igPopFont();

  igSpacing();

  // 4. Secondary Buttons Row: Aspectos (Skin editor) & Ajustes (Settings)
  float half_btn_w = (menu_w - style->ItemSpacing.x) * 0.5f;
  igSetCursorPosX(center_x);
  igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
             usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);

  igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.20f, 0.24f, 0.32f, 1.0f});
  igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.26f, 0.32f, 0.42f, 1.0f});

  if (igButton("\ue90c  Aspectos", (ImVec2){half_btn_w, 52.0f})) {
    usr->gdata.curr_screen = SKIN_EDITOR;
  }
  igSameLine(0, -1);

  if (igButton("\ue991  Ajustes", (ImVec2){half_btn_w, 52.0f})) {
    usr->gdata.curr_screen = SETTINGS;
  }

  // 5. Salir (Quit) Button
  igSetCursorPosX(center_x);
  igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.18f, 0.12f, 0.14f, 1.0f});
  igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.28f, 0.16f, 0.18f, 1.0f});
  if (igButton("\ue9b6  Salir", (ImVec2){menu_w, 46.0f})) {
    env->config.running = false;
    save_user_settings(usrs);
  }
  igPopStyleColor(4);
  igPopFont();

  igPopStyleVar(2);

  // Floating modals
  ui_virtual_keyboard(env);
  ui_server_selector(env);
}

void ui_title_screen_destroy(tenv* env) {
  (void)env;
}
