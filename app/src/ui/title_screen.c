#include "title_screen.h"

#include <stdio.h>
#include <string.h>

#include "../network/server.h"
#include "../network/server_list.h"
#include "controls_editor.h"
#include "bot_settings.h"
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

  static int s_selected_zone = 1; // 0=Todas, 1=🌧️ Lluvia (SnakeyRain), 2=Norteamérica, 3=Europa, 4=Sudamérica, 5=Asia/Otros

  ImVec2 modal_sz = {fminf(740.0f, ctx->size[0] * 0.95f), fminf(600.0f, ctx->size[1] * 0.94f)};
  igSetNextWindowSize(modal_sz, ImGuiCond_Always);
  igSetNextWindowPos((ImVec2){(ctx->size[0] - modal_sz.x) * 0.5f,
                             (ctx->size[1] - modal_sz.y) * 0.5f},
                     ImGuiCond_Always, (ImVec2){});

  igPushStyleColor_Vec4(ImGuiCol_WindowBg, (ImVec4){0.09f, 0.11f, 0.15f, 0.98f});
  igPushStyleColor_Vec4(ImGuiCol_Border, (ImVec4){0.25f, 0.55f, 0.90f, 0.70f});
  igPushStyleVar_Float(ImGuiStyleVar_WindowRounding, 16.0f);
  igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 2.0f);

  if (igBegin("##server_modal", NULL,
              ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
    igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_LARGE],
               usr->imgui_data.regular_font_bold[FONT_SIZE_LARGE]->LegacySize);
    igTextColored((ImVec4){0.35f, 0.70f, 1.0f, 1.0f}, "Seleccionar Servidor y Zona");
    igPopFont();

    igTextColored((ImVec4){0.70f, 0.75f, 0.80f, 1.0f},
                  "Servidores oficiales y Zona Lluvia (Radar en directo https://snakeyrain.com/weather/):");
    igSeparator();
    igSpacing();

    // 1. Zone filter tabs (Todas, 🌧️ Zona Lluvia, Norteamérica, Europa, Sudamérica, Asia/Otros)
    igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
               usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
    igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 8.0f);

    const char* zone_titles[] = {"Todas", "🌧️ Zona Lluvia", "Norteamérica", "Europa", "Sudamérica", "Asia"};
    int num_zones = 6;
    float avail_w = modal_sz.x - 30.0f;
    float tab_spacing = 6.0f;
    float tab_w = (avail_w - (num_zones - 1) * tab_spacing) / num_zones;

    for (int z = 0; z < num_zones; z++) {
      if (z > 0) igSameLine(0, tab_spacing);
      bool is_tab_active = (s_selected_zone == z);

      if (z == 1) { // 🌧️ Zona Lluvia (SnakeyRain Pink/Magenta style)
        if (is_tab_active) {
          igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.88f, 0.18f, 0.52f, 1.0f});
          igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.98f, 0.28f, 0.62f, 1.0f});
        } else {
          igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.32f, 0.10f, 0.22f, 0.90f});
          igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.52f, 0.16f, 0.36f, 1.0f});
        }
      } else {
        if (is_tab_active) {
          igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.20f, 0.50f, 0.85f, 1.0f});
          igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.28f, 0.60f, 0.95f, 1.0f});
        } else {
          igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.14f, 0.17f, 0.23f, 0.85f});
          igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.22f, 0.28f, 0.38f, 1.0f});
        }
      }

      if (igButton(zone_titles[z], (ImVec2){tab_w, 34.0f})) {
        s_selected_zone = z;
      }
      igPopStyleColor(2);
    }
    igPopStyleVar(1);
    igPopFont();

    igSpacing();

    // 2. Weather Status Banner if in Lluvia zone
    if (s_selected_zone == 1) {
      int rain_srv_count = snakeyrain_weather_count();
      int total_storm_bots = snakeyrain_weather_total_bots();
      bool ws_live = snakeyrain_weather_is_connected();

      igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
                 usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
      igTextColored((ImVec4){1.0f, 0.40f, 0.70f, 1.0f}, "🌧️ SnakeyRain Weather:");
      igSameLine(0, 6.0f);
      if (ws_live) {
        igTextColored((ImVec4){0.20f, 0.95f, 0.50f, 1.0f}, "En Vivo (%d tormentas activas, %d bots)",
                      rain_srv_count, total_storm_bots);
      } else {
        igTextColored((ImVec4){0.95f, 0.85f, 0.25f, 1.0f}, "Radar Sincronizado (%d tormentas, %d bots)",
                      rain_srv_count, total_storm_bots);
      }
      igPopFont();
      igSpacing();
    }

    // 3. Quick Action Buttons: Auto Best Ping & Refresh
    igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
               usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
    igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 10.0f);

    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.15f, 0.65f, 0.35f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.20f, 0.80f, 0.42f, 1.0f});
    const char* best_btn_txt = (s_selected_zone == 1) ? "Mejor Ping en Lluvia" : "Mejor Ping Automático";
    if (igButton(best_btn_txt, (ImVec2){modal_sz.x * 0.52f, 42.0f})) {
      const char* best_ip = (s_selected_zone == 1) ?
                            server_list_get_best_ip_by_region("Lluvia") :
                            server_list_get_best_ip();
      if (best_ip && best_ip[0] != '\0') {
        strncpy(usrs->ipv4, best_ip, MAX_IPV4_LEN);
        save_user_settings(usrs);
        s_show_server_selector = false;
      }
    }
    igPopStyleColor(2);
    igSameLine(0, 10.0f);

    const char* refresh_btn_txt = (s_selected_zone == 1) ? "Actualizar Radar Lluvia" : "Actualizar Pings";
    if (igButton(refresh_btn_txt, (ImVec2){-1, 42.0f})) {
      server_list_refresh_pings();
    }
    igPopStyleVar(1);
    igPopFont();

    igSpacing();

    // 4. Server list scrollable container
    float list_h = modal_sz.y - (s_selected_zone == 1 ? 230.0f : 205.0f);
    if (igBeginChild_Str("##srv_list", (ImVec2){-1, list_h}, true, ImGuiWindowFlags_None)) {
      int count = server_list_count();
      int shown_cards = 0;

      for (int i = 0; i < count; i++) {
        server_entry s_buf;
        if (!server_list_get_copy(i, &s_buf)) continue;
        server_entry* s = &s_buf;

        // Zone filtering logic
        bool is_rain = (strcmp(s->region, "Lluvia") == 0);
        bool matches = false;
        if (s_selected_zone == 0) {
          matches = true; // Todas
        } else if (s_selected_zone == 1) {
          matches = is_rain; // 🌧️ Zona Lluvia
        } else if (s_selected_zone == 2) {
          matches = (!is_rain && (strcmp(s->region, "US-W") == 0 || strcmp(s->region, "US-C") == 0 ||
                                  strcmp(s->region, "US-E") == 0 || strcmp(s->region, "US-S") == 0 ||
                                  strcmp(s->region, "NA") == 0));
        } else if (s_selected_zone == 3) {
          matches = (!is_rain && strcmp(s->region, "EU") == 0);
        } else if (s_selected_zone == 4) {
          matches = (!is_rain && strcmp(s->region, "SA") == 0);
        } else if (s_selected_zone == 5) {
          matches = (!is_rain && (strcmp(s->region, "AS") == 0 || strcmp(s->region, "ME") == 0 ||
                                  strcmp(s->region, "AF") == 0));
        }
        if (!matches) continue;
        shown_cards++;

        char srv_addr[64];
        snprintf(srv_addr, sizeof(srv_addr), "%s:%d", s->ip, s->port);
        bool is_selected = (strcmp(usrs->ipv4, srv_addr) == 0 || strcmp(usrs->ipv4, s->full_addr) == 0);

        igPushID_Int(i);
        if (is_selected) {
          igPushStyleColor_Vec4(ImGuiCol_ChildBg, is_rain ?
                                (ImVec4){0.35f, 0.12f, 0.28f, 0.85f} :
                                (ImVec4){0.15f, 0.28f, 0.40f, 0.80f});
        } else {
          igPushStyleColor_Vec4(ImGuiCol_ChildBg, is_rain ?
                                (ImVec4){0.16f, 0.11f, 0.18f, 0.70f} :
                                (ImVec4){0.12f, 0.14f, 0.18f, 0.60f});
        }
        igPushStyleVar_Float(ImGuiStyleVar_ChildRounding, 8.0f);

        float card_h = is_rain ? 60.0f : 54.0f;
        char child_id[32];
        snprintf(child_id, sizeof(child_id), "srv_card_%d", i);
        if (igBeginChild_Str(child_id, (ImVec2){-1, card_h}, true, ImGuiWindowFlags_None)) {
          // Ping badge
          igSetCursorPos((ImVec2){10, is_rain ? 18 : 14});
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

          // SID badge
          igSameLine(115, -1);
          igSetCursorPosY(is_rain ? 10 : 14);
          if (is_rain) {
            igTextColored((ImVec4){1.0f, 0.40f, 0.75f, 1.0f}, "#%s", s->sid_str[0] ? s->sid_str : "----");
          } else if (s->sid > 0) {
            igTextColored((ImVec4){0.40f, 0.80f, 0.95f, 1.0f}, "#%04d", s->sid);
          } else {
            igTextColored((ImVec4){0.40f, 0.80f, 0.95f, 0.4f}, "#----");
          }

          // Server details
          if (is_rain) {
            // Row 1: City & Continent
            igSameLine(175, -1);
            igSetCursorPosY(10);
            igTextColored((ImVec4){1.0f, 1.0f, 1.0f, 1.0f}, "🌧️ %s (%s)", s->city, s->cont);

            // Row 2: Storm details (Bots, State, Players, Address)
            igSetCursorPos((ImVec2){175, 34});
            igTextColored((ImVec4){1.0f, 0.35f, 0.65f, 1.0f}, "🌧️ %d Bots", s->bots_alive);

            igSameLine(280, -1);
            igSetCursorPosY(34);
            if (strcmp(s->storm_state, "rain") == 0) {
              igTextColored((ImVec4){0.20f, 0.90f, 1.0f, 1.0f}, "[TORMENTA]");
            } else {
              igTextColored((ImVec4){0.80f, 0.60f, 1.0f, 1.0f}, "[%s]", s->storm_state);
            }

            igSameLine(380, -1);
            igSetCursorPosY(34);
            igTextColored((ImVec4){0.65f, 0.70f, 0.80f, 1.0f}, "👥 %d Jug.", s->players);

            igSameLine(480, -1);
            igSetCursorPosY(34);
            igTextColored((ImVec4){0.50f, 0.55f, 0.65f, 1.0f}, "%s", srv_addr);
          } else {
            // Standard official server card
            igSameLine(175, -1);
            igSetCursorPosY(14);
            igTextColored((ImVec4){1.0f, 1.0f, 1.0f, 1.0f}, "%s", s->name);

            igSameLine(390, -1);
            igSetCursorPosY(14);
            igTextColored((ImVec4){0.60f, 0.65f, 0.70f, 1.0f}, "%s", srv_addr);
          }

          // Select button
          igSameLine(-1, -1);
          igSetCursorPos((ImVec2){modal_sz.x - 145.0f, is_rain ? 12.0f : 8.0f});
          igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 6.0f);
          if (is_selected) {
            igPushStyleColor_Vec4(ImGuiCol_Button, is_rain ?
                                  (ImVec4){0.70f, 0.15f, 0.40f, 1.0f} :
                                  (ImVec4){0.10f, 0.45f, 0.25f, 1.0f});
            igButton("Activo", (ImVec2){95, 36});
            igPopStyleColor(1);
          } else {
            if (igButton("Elegir", (ImVec2){95, 36})) {
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

      if (shown_cards == 0) {
        igSetCursorPos((ImVec2){20, 20});
        igTextColored((ImVec4){0.70f, 0.70f, 0.70f, 1.0f}, "No hay servidores disponibles en esta zona actualmente.");
      }
    }
    igEndChild();

    igSpacing();
    igSetCursorPosX((modal_sz.x - 160.0f) * 0.5f);
    igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 8.0f);
    if (igButton("Cerrar", (ImVec2){160.0f, 40.0f})) {
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

  // Top-left: Debug & Telemetry status toggle pill
  igSetCursorPos((ImVec2){24.0f, 14.0f});
  igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_SMALL],
             usr->imgui_data.regular_font_bold[FONT_SIZE_SMALL]->LegacySize);
  igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 8.0f);
  if (usrs->debug_logs_enabled) {
    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.12f, 0.38f, 0.45f, 0.90f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.16f, 0.48f, 0.58f, 1.0f});
    if (igButton("LOGS & DEBUG: ACTIVO", (ImVec2){200.0f, 32.0f})) {
      usrs->debug_logs_enabled = false;
      save_user_settings(usrs);
    }
  } else {
    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.20f, 0.22f, 0.26f, 0.70f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.28f, 0.30f, 0.35f, 0.90f});
    if (igButton("LOGS & DEBUG: INACTIVO", (ImVec2){200.0f, 32.0f})) {
      usrs->debug_logs_enabled = true;
      save_user_settings(usrs);
    }
  }
  igPopStyleColor(2);
  igPopStyleVar(1);
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

  // Stats row with icons: Trophy/Star (\ue99e), Kills (\ueaeb), Time (\ue952)
  int tot_sec = (int)usrs->play_time;
  int hours = tot_sec / 3600;
  int minutes = (tot_sec % 3600) / 60;
  int seconds = tot_sec % 60;

  char score_buf[32], kills_buf[32], time_buf[32];
  snprintf(score_buf, sizeof(score_buf), " %d", usrs->score);
  snprintf(kills_buf, sizeof(kills_buf), " %d", usrs->kills);
  snprintf(time_buf, sizeof(time_buf), " %02d:%02d:%02d", hours, minutes, seconds);

  ImVec2 sz_sc_ic, sz_sc_val, sz_k_ic, sz_k_val, sz_t_ic, sz_t_val;
  igCalcTextSize(&sz_sc_ic, "\ue99e", NULL, false, -1);
  igCalcTextSize(&sz_sc_val, score_buf, NULL, false, -1);
  igCalcTextSize(&sz_k_ic, "\ueaeb", NULL, false, -1);
  igCalcTextSize(&sz_k_val, kills_buf, NULL, false, -1);
  igCalcTextSize(&sz_t_ic, "\ue952", NULL, false, -1);
  igCalcTextSize(&sz_t_val, time_buf, NULL, false, -1);

  float total_stats_w = sz_sc_ic.x + sz_sc_val.x + 28.0f +
                        sz_k_ic.x + sz_k_val.x + 28.0f +
                        sz_t_ic.x + sz_t_val.x;
  igSetCursorPosX((ctx->size[0] - total_stats_w) * 0.5f);
  igSpacing();

  // Record: Golden Trophy/Star
  igTextColored((ImVec4){0.95f, 0.82f, 0.20f, 1.0f}, "\ue99e");
  igSameLine(0, 4.0f);
  igTextColored((ImVec4){0.90f, 0.92f, 0.95f, 0.95f}, "%s", score_buf);

  // Separator
  igSameLine(0, 14.0f);
  igTextColored((ImVec4){0.4f, 0.45f, 0.5f, 0.6f}, "|");

  // Kills: Crimson Skull
  igSameLine(0, 14.0f);
  igTextColored((ImVec4){0.95f, 0.30f, 0.30f, 1.0f}, "\ueaeb");
  igSameLine(0, 4.0f);
  igTextColored((ImVec4){0.90f, 0.92f, 0.95f, 0.95f}, "%s", kills_buf);

  // Separator
  igSameLine(0, 14.0f);
  igTextColored((ImVec4){0.4f, 0.45f, 0.5f, 0.6f}, "|");

  // Tiempo: Cyan Clock
  igSameLine(0, 14.0f);
  igTextColored((ImVec4){0.30f, 0.75f, 0.95f, 1.0f}, "\ue952");
  igSameLine(0, 4.0f);
  igTextColored((ImVec4){0.90f, 0.92f, 0.95f, 0.95f}, "%s", time_buf);

  igSpacing();
  igSpacing();

  // Style rounding for modern touch controls
  igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 12.0f);
  igPushStyleVar_Vec2(ImGuiStyleVar_ItemSpacing, (ImVec2){12.0f, 12.0f});

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
    snprintf(srv_btn_label, sizeof(srv_btn_label), "Servidor: %s (%d ms)  [v]", srv_name, cur_ping);
  } else {
    snprintf(srv_btn_label, sizeof(srv_btn_label), "Servidor: %s  [v]", srv_name);
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
    server_disconnect(env);
    usr->gdata.conn = CONNECTING;
    usr->gdata.curr_screen = PLAYING;
    usr->gdata.connect_retry_count = 0;

    // If server is invalid or empty or obsolete default, use reliable Silicon Valley default
    if (usrs->ipv4[0] == '\0' || strcmp(usrs->ipv4, "192.211.52.146:444") == 0) {
      strncpy(usrs->ipv4, "23.29.125.178:444", MAX_IPV4_LEN);
    }

    usrs->hotkeys[HOTKEY_BOT].active = usrs->bot_auto_start; // False by default
    server_connect(env);
  }
  igPopStyleColor(3);
  igPopFont();

  igSpacing();

  // 4. MODO BOT Button (Prominent Bot Configuration Entry)
  igSetCursorPosX(center_x);
  igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
             usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
  igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.18f, 0.36f, 0.52f, 1.0f});
  igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.24f, 0.46f, 0.66f, 1.0f});
  const char* cur_mode_tag = (usrs->bot_mode == 1) ? "Ataque/Caza" : (usrs->bot_mode == 2 ? "Auto-Coil" : "Ultra-Defensivo");
  char bot_btn_label[64];
  snprintf(bot_btn_label, sizeof(bot_btn_label), "Modo Bot: %s", cur_mode_tag);
  if (igButton(bot_btn_label, (ImVec2){menu_w, 52.0f})) {
    usr->gdata.curr_screen = BOT_SETTINGS;
  }
  igPopStyleColor(2);
  igPopFont();

  igSpacing();

  // 5. Button: Toggle Logs & Debug
  igSetCursorPosX(center_x);
  igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
             usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
  if (usrs->debug_logs_enabled) {
    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.14f, 0.38f, 0.44f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.18f, 0.48f, 0.55f, 1.0f});
    if (igButton("Logs y Debug: ACTIVADO (Desactivar)", (ImVec2){menu_w, 48.0f})) {
      usrs->debug_logs_enabled = false;
      save_user_settings(usrs);
    }
  } else {
    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.22f, 0.24f, 0.28f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.28f, 0.32f, 0.38f, 1.0f});
    if (igButton("Logs y Debug: DESACTIVADO (Activar)", (ImVec2){menu_w, 48.0f})) {
      usrs->debug_logs_enabled = true;
      save_user_settings(usrs);
    }
  }
  igPopStyleColor(2);
  igPopFont();

  igSpacing();

  // 6. Secondary Buttons Row: Controles, Aspectos, Ajustes
  float third_btn_w = (menu_w - 2.0f * style->ItemSpacing.x) / 3.0f;
  igSetCursorPosX(center_x);
  igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
             usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);

  igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.18f, 0.28f, 0.40f, 1.0f});
  igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.24f, 0.38f, 0.54f, 1.0f});

  if (igButton("\ueaed  Controles", (ImVec2){third_btn_w, 50.0f})) {
    ui_controls_editor_init(env);
    usr->gdata.curr_screen = CONTROLS_EDITOR;
  }
  igSameLine(0, -1);

  if (igButton("\ue90c  Aspectos", (ImVec2){third_btn_w, 50.0f})) {
    usr->gdata.curr_screen = SKIN_EDITOR;
  }
  igSameLine(0, -1);

  if (igButton("\ue991  Ajustes", (ImVec2){third_btn_w, 50.0f})) {
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
