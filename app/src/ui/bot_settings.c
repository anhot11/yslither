#include "bot_settings.h"

#include <stdio.h>
#include <string.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "../cimgui/cimgui.h"
#include "../user.h"
#include "../game/flight_recorder.h"

void ui_bot_settings_init(tenv* env) {
  (void)env;
}

void ui_bot_settings(tenv* env) {
  tuser_data* usr = env->usr;
  tcontext* ctx = env->ctx;
  user_settings* usrs = &usr->usrs;
  float screen_w = (float)ctx->size[0];
  float screen_h = (float)ctx->size[1];

  // Header Title
  igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_LARGE],
             usr->imgui_data.regular_font_bold[FONT_SIZE_LARGE]->LegacySize);
  igSetCursorPos((ImVec2){24.0f, 16.0f});
  igTextColored((ImVec4){0.20f, 0.90f, 0.50f, 1.0f}, "🤖  CONFIGURACION DEL MODO BOT & IA");
  igPopFont();

  igPushFont(usr->imgui_data.regular_font[FONT_SIZE_SMALL],
             usr->imgui_data.regular_font[FONT_SIZE_SMALL]->LegacySize);
  igSetCursorPos((ImVec2){24.0f, 50.0f});
  igTextColored((ImVec4){0.70f, 0.75f, 0.80f, 0.90f},
                "Elige el algoritmo de navegacion, activa las lineas visuales y zonas rojas de peligro.");
  igPopFont();

  float top_h = 76.0f;
  float bottom_bar_h = 64.0f;
  float content_h = screen_h - top_h - bottom_bar_h;
  float col_w = (screen_w - 48.0f - 16.0f) * 0.5f;

  igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 10.0f);
  igPushStyleVar_Vec2(ImGuiStyleVar_ItemSpacing, (ImVec2){10.0f, 10.0f});

  // LEFT COLUMN: MODO DEL BOT (ALGORITMO)
  igSetCursorPos((ImVec2){24.0f, top_h});
  igPushStyleColor_Vec4(ImGuiCol_ChildBg, (ImVec4){0.10f, 0.12f, 0.16f, 0.95f});
  igPushStyleColor_Vec4(ImGuiCol_Border, (ImVec4){0.20f, 0.28f, 0.38f, 0.80f});
  igPushStyleVar_Float(ImGuiStyleVar_ChildRounding, 12.0f);
  igPushStyleVar_Float(ImGuiStyleVar_ChildBorderSize, 1.5f);

  if (igBeginChild_Str("##bot_modes_col", (ImVec2){col_w, content_h}, true, ImGuiWindowFlags_None)) {
    igSpacing();
    igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
               usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
    igTextColored((ImVec4){0.35f, 0.75f, 1.0f, 1.0f}, "MODO DE COMPORTAMIENTO");
    igPopFont();
    igSeparator();
    igSpacing();

    // Mode 0: Ultra-Defensivo
    bool is_m0 = (usrs->bot_mode == 0);
    if (is_m0) {
      igPushStyleColor_Vec4(ImGuiCol_ChildBg, (ImVec4){0.12f, 0.26f, 0.18f, 0.90f});
      igPushStyleColor_Vec4(ImGuiCol_Border, (ImVec4){0.25f, 0.85f, 0.45f, 1.0f});
    } else {
      igPushStyleColor_Vec4(ImGuiCol_ChildBg, (ImVec4){0.13f, 0.15f, 0.20f, 0.70f});
      igPushStyleColor_Vec4(ImGuiCol_Border, (ImVec4){0.22f, 0.25f, 0.32f, 0.80f});
    }

    if (igBeginChild_Str("##mode_card_0", (ImVec2){-1, 108.0f}, true, ImGuiWindowFlags_None)) {
      igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
                 usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
      igTextColored(is_m0 ? (ImVec4){0.30f, 0.95f, 0.55f, 1.0f} : (ImVec4){0.85f, 0.85f, 0.85f, 1.0f},
                    "🛡️  Ultra-Defensivo / Supervivencia");
      igPopFont();

      igPushFont(usr->imgui_data.regular_font[FONT_SIZE_SMALL],
                 usr->imgui_data.regular_font[FONT_SIZE_SMALL]->LegacySize);
      igTextColored((ImVec4){0.70f, 0.75f, 0.80f, 0.90f},
                    "Radar de 32 sectores. Maxima distancia de seguridad, evasiva inteligente y recoleccion prudente de masa sin exponerse a trampas.");
      igPopFont();

      igSpacing();
      if (is_m0) {
        igTextColored((ImVec4){0.25f, 0.90f, 0.50f, 1.0f}, "✓ SELECCIONADO");
      } else {
        if (igButton("Activar Modo Defensivo", (ImVec2){180.0f, 26.0f})) {
          usrs->bot_mode = 0;
        }
      }
    }
    igEndChild();
    igPopStyleColor(2);

    igSpacing();

    // Mode 1: Ataque / Caza & Crecimiento (Recomendado)
    bool is_m1 = (usrs->bot_mode == 1);
    if (is_m1) {
      igPushStyleColor_Vec4(ImGuiCol_ChildBg, (ImVec4){0.28f, 0.20f, 0.10f, 0.90f});
      igPushStyleColor_Vec4(ImGuiCol_Border, (ImVec4){0.95f, 0.65f, 0.20f, 1.0f});
    } else {
      igPushStyleColor_Vec4(ImGuiCol_ChildBg, (ImVec4){0.13f, 0.15f, 0.20f, 0.70f});
      igPushStyleColor_Vec4(ImGuiCol_Border, (ImVec4){0.22f, 0.25f, 0.32f, 0.80f});
    }

    if (igBeginChild_Str("##mode_card_1", (ImVec2){-1, 108.0f}, true, ImGuiWindowFlags_None)) {
      igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
                 usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
      igTextColored(is_m1 ? (ImVec4){0.98f, 0.75f, 0.25f, 1.0f} : (ImVec4){0.85f, 0.85f, 0.85f, 1.0f},
                    "⚔️  Ataque / Caza & Crecimiento");
      igPopFont();

      igPushFont(usr->imgui_data.regular_font[FONT_SIZE_SMALL],
                 usr->imgui_data.regular_font[FONT_SIZE_SMALL]->LegacySize);
      igTextColored((ImVec4){0.70f, 0.75f, 0.80f, 0.90f},
                    "Busqueda activa de comida para crecer sin morir. Prioriza alimento cercano, aspira estelas de serpientes caidas y mantiene blindaje defensivo total.");
      igPopFont();

      igSpacing();
      if (is_m1) {
        igTextColored((ImVec4){0.95f, 0.70f, 0.20f, 1.0f}, "✓ SELECCIONADO (POR DEFECTO)");
      } else {
        if (igButton("Activar Modo Ataque", (ImVec2){180.0f, 26.0f})) {
          usrs->bot_mode = 1;
        }
      }
    }
    igEndChild();
    igPopStyleColor(2);

    igSpacing();

    // Mode 2: Auto-Coil Continuo
    bool is_m2 = (usrs->bot_mode == 2);
    if (is_m2) {
      igPushStyleColor_Vec4(ImGuiCol_ChildBg, (ImVec4){0.22f, 0.12f, 0.28f, 0.90f});
      igPushStyleColor_Vec4(ImGuiCol_Border, (ImVec4){0.75f, 0.35f, 0.95f, 1.0f});
    } else {
      igPushStyleColor_Vec4(ImGuiCol_ChildBg, (ImVec4){0.13f, 0.15f, 0.20f, 0.70f});
      igPushStyleColor_Vec4(ImGuiCol_Border, (ImVec4){0.22f, 0.25f, 0.32f, 0.80f});
    }

    if (igBeginChild_Str("##mode_card_2", (ImVec2){-1, 108.0f}, true, ImGuiWindowFlags_None)) {
      igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
                 usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
      igTextColored(is_m2 ? (ImVec4){0.85f, 0.45f, 0.98f, 1.0f} : (ImVec4){0.85f, 0.85f, 0.85f, 1.0f},
                    "🌀  Auto-Coil Continuo");
      igPopFont();

      igPushFont(usr->imgui_data.regular_font[FONT_SIZE_SMALL],
                 usr->imgui_data.regular_font[FONT_SIZE_SMALL]->LegacySize);
      igTextColored((ImVec4){0.70f, 0.75f, 0.80f, 0.90f},
                    "Tactica de enrollamiento defensivo. Se enrosca sobre su propio cuerpo en circulo protector para mantener puntuacion y sobrevivir.");
      igPopFont();

      igSpacing();
      if (is_m2) {
        igTextColored((ImVec4){0.80f, 0.40f, 0.95f, 1.0f}, "✓ SELECCIONADO");
      } else {
        if (igButton("Activar Modo Auto-Coil", (ImVec2){180.0f, 26.0f})) {
          usrs->bot_mode = 2;
        }
      }
    }
    igEndChild();
    igPopStyleColor(2);

    igSpacing();
    igSeparator();
    igSpacing();

    // Behavior check toggles
    igCheckbox("Permitir Turbo Automatico Inteligente", &usrs->bot_auto_turbo);
    igTextColored((ImVec4){0.60f, 0.65f, 0.70f, 0.85f},
                  "Acelera para escapar de encierros criticos o atrapar comida.");

    igSpacing();
    igCheckbox("Activar Bot Automaticamente al Iniciar Partida", &usrs->bot_auto_start);
    igTextColored((ImVec4){0.60f, 0.65f, 0.70f, 0.85f},
                  "El bot tomara el control en cuanto te conectes al servidor.");

    igSpacing();
    igSeparator();
    igSpacing();

    // Flight Recorder / Telemetry / Debug Master Switch
    igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
               usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
    igTextColored((ImVec4){0.35f, 0.85f, 0.95f, 1.0f}, "CAJA NEGRA Y MODO DEBUG");
    igPopFont();
    igSpacing();

    igCheckbox("Sistema de Logs & Modo Debug (Caja Negra)", &usrs->debug_logs_enabled);
    igTextColored((ImVec4){0.60f, 0.65f, 0.70f, 0.85f},
                  "Almacena telemetria en memoria (300 frames) y genera diagnostico de muerte al chocar en telemetry_death_latest.json.");

    if (flight_recorder_has_death_event()) {
      igSpacing();
      igPushStyleColor_Vec4(ImGuiCol_ChildBg, (ImVec4){0.20f, 0.12f, 0.14f, 0.90f});
      igPushStyleColor_Vec4(ImGuiCol_Border, (ImVec4){0.80f, 0.30f, 0.30f, 0.80f});
      if (igBeginChild_Str("##death_diag", (ImVec2){-1, 62.0f}, true, ImGuiWindowFlags_None)) {
        igTextColored((ImVec4){0.95f, 0.40f, 0.40f, 1.0f}, "Ultimo Diagnostico Registrado:");
        igTextWrapped("%s", flight_recorder_get_last_death_summary());
      }
      igEndChild();
      igPopStyleColor(2);
    }
  }
  igEndChild();
  igPopStyleVar(2);
  igPopStyleColor(2);

  // RIGHT COLUMN: VISUALIZACIONES EN PANTALLA & SENSIBILIDAD
  igSetCursorPos((ImVec2){24.0f + col_w + 16.0f, top_h});
  igPushStyleColor_Vec4(ImGuiCol_ChildBg, (ImVec4){0.10f, 0.12f, 0.16f, 0.95f});
  igPushStyleColor_Vec4(ImGuiCol_Border, (ImVec4){0.20f, 0.28f, 0.38f, 0.80f});
  igPushStyleVar_Float(ImGuiStyleVar_ChildRounding, 12.0f);
  igPushStyleVar_Float(ImGuiStyleVar_ChildBorderSize, 1.5f);

  if (igBeginChild_Str("##bot_visuals_col", (ImVec2){col_w, content_h}, true, ImGuiWindowFlags_None)) {
    igSpacing();
    igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
               usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
    igTextColored((ImVec4){0.35f, 0.75f, 1.0f, 1.0f}, "ELEMENTOS VISUALES EN PANTALLA");
    igPopFont();
    igSeparator();
    igSpacing();

    // 1. Bot Target Line
    igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
               usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
    igCheckbox("Linea de Objetivo del Bot (Cyan)", &usrs->bot_visual_line);
    igPopFont();
    igIndent(26.0f);
    igPushFont(usr->imgui_data.regular_font[FONT_SIZE_SMALL],
               usr->imgui_data.regular_font[FONT_SIZE_SMALL]->LegacySize);
    igTextColored((ImVec4){0.30f, 0.85f, 0.95f, 0.90f},
                  "Trazo brillante desde la cabeza hasta el punto de mira hacia donde se dirige el bot.");
    igPopFont();
    igIndent(-26.0f);

    igSpacing();

    // 2. Danger Red Zones
    igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
               usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
    igCheckbox("Zonas Rojas de Peligro y Colision (Rojo)", &usrs->bot_visual_zones);
    igPopFont();
    igIndent(26.0f);
    igPushFont(usr->imgui_data.regular_font[FONT_SIZE_SMALL],
               usr->imgui_data.regular_font[FONT_SIZE_SMALL]->LegacySize);
    igTextColored((ImVec4){0.95f, 0.35f, 0.35f, 0.90f},
                  "Circulos rojos alrededor de cabezas enemigas proyectadas, cuerpos cercanos y limites del mapa.");
    igPopFont();
    igIndent(-26.0f);

    igSpacing();

    // 3. Sensor feelers radar
    igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
               usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
    igCheckbox("Radar de Sensores y Deteccion (Amarillo / Naranja)", &usrs->bot_visual_radar);
    igPopFont();
    igIndent(26.0f);
    igPushFont(usr->imgui_data.regular_font[FONT_SIZE_SMALL],
               usr->imgui_data.regular_font[FONT_SIZE_SMALL]->LegacySize);
    igTextColored((ImVec4){0.95f, 0.80f, 0.25f, 0.90f},
                  "Esferas de deteccion frontal y feelers laterales que calculan las curvas seguras.");
    igPopFont();
    igIndent(-26.0f);

    igSpacing();

    // 4. Food Target Line
    igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
               usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
    igCheckbox("Linea hacia Objetivo de Comida (Verde)", &usrs->bot_visual_food);
    igPopFont();
    igIndent(26.0f);
    igPushFont(usr->imgui_data.regular_font[FONT_SIZE_SMALL],
               usr->imgui_data.regular_font[FONT_SIZE_SMALL]->LegacySize);
    igTextColored((ImVec4){0.35f, 0.90f, 0.45f, 0.90f},
                  "Destaca el pellet de alimento con mayor tamano y seguridad seleccionado por la IA.");
    igPopFont();
    igIndent(-26.0f);

    igSpacing();
    igSeparator();
    igSpacing();

    // Sliders
    igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
               usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
    igTextColored((ImVec4){0.35f, 0.75f, 1.0f, 1.0f}, "SENSIBILIDAD Y DISTANCIAS");
    igPopFont();
    igSpacing();

    igText("Margen de Seguridad (Multiplicador de Radio): %dx", usrs->bot_radius_mult);
    igSliderInt("##bot_radius_mult", &usrs->bot_radius_mult, 10, 40, "%d", ImGuiSliderFlags_None);
    igTextColored((ImVec4){0.60f, 0.65f, 0.70f, 0.85f},
                  "Mayor valor = la serpiente evade enemigos desde mucho mas lejos.");

    igSpacing();
    igText("Puntuacion Minima para Auto-Coil: %d pts", usrs->bot_follow_circle_score);
    igSliderInt("##bot_circle_score", &usrs->bot_follow_circle_score, 500, 10000, "%d", ImGuiSliderFlags_None);
    igTextColored((ImVec4){0.60f, 0.65f, 0.70f, 0.85f},
                  "Masa necesaria para activar el repliegue defensivo circular.");
  }
  igEndChild();
  igPopStyleVar(2);
  igPopStyleColor(2);

  // BOTTOM BAR
  float bottom_y = screen_h - bottom_bar_h + 8.0f;
  igSetCursorPos((ImVec2){24.0f, bottom_y});

  // Restore Defaults Button
  igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.20f, 0.22f, 0.28f, 1.0f});
  igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.28f, 0.32f, 0.40f, 1.0f});
  if (igButton("↺  Valores por Defecto", (ImVec2){220.0f, 48.0f})) {
    usrs->bot_mode = 1; // Ataque / Caza por defecto
    usrs->bot_visual_line = true;
    usrs->bot_visual_zones = true;
    usrs->bot_visual_radar = true;
    usrs->bot_visual_food = true;
    usrs->bot_auto_turbo = true;
    usrs->bot_auto_start = false;
    usrs->bot_radius_mult = 20;
    usrs->bot_follow_circle_score = 2000;
    usrs->debug_logs_enabled = true;
  }
  igPopStyleColor(2);

  // Save & Return Button
  float save_w = 260.0f;
  igSetCursorPos((ImVec2){screen_w - save_w - 24.0f, bottom_y});
  igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.14f, 0.70f, 0.35f, 1.0f});
  igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.18f, 0.82f, 0.42f, 1.0f});
  igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
             usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
  if (igButton("✓  Guardar y Volver al Menu", (ImVec2){save_w, 48.0f})) {
    save_user_settings(usrs);
    usr->gdata.curr_screen = TITLE_SCREEN;
  }
  igPopFont();
  igPopStyleColor(2);

  igPopStyleVar(2);
}

void ui_bot_settings_destroy(tenv* env) {
  (void)env;
}
