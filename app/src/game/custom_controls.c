#include "custom_controls.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "../cimgui/cimgui.h"
#include "../user.h"

#define CUSTOM_CONTROLS_FILE "custom_controls.dat"

custom_controls_t g_custom_controls;

const char* custom_controls_action_name(button_action_t action) {
  switch (action) {
    case BTN_ACTION_BOOST:    return "Turbo / Acelerar";
    case BTN_ACTION_ZOOM_IN:  return "Acercar Zoom (Tecla N)";
    case BTN_ACTION_ZOOM_OUT: return "Alejar Zoom (Tecla M)";
    case BTN_ACTION_BOT:      return "Bot Defensivo (Tecla T)";
    case BTN_ACTION_ASSIST:   return "Modo Asistencia (Tecla K)";
    case BTN_ACTION_RESTART:  return "Reiniciar Partida (Tecla R)";
    case BTN_ACTION_NAMES:    return "Mostrar Nombres (Tecla P)";
    case BTN_ACTION_BIG_FOOD: return "Comida Grande (Tecla F)";
    case BTN_ACTION_HUD:      return "Alternar HUD (Tecla H)";
    case BTN_ACTION_QUIT:     return "Salir al Menú (Tecla Q)";
    default:                  return "Ninguna";
  }
}

const char* custom_controls_action_key_str(button_action_t action) {
  switch (action) {
    case BTN_ACTION_BOOST:    return "Espacio";
    case BTN_ACTION_ZOOM_IN:  return "N";
    case BTN_ACTION_ZOOM_OUT: return "M";
    case BTN_ACTION_BOT:      return "T";
    case BTN_ACTION_ASSIST:   return "K";
    case BTN_ACTION_RESTART:  return "R";
    case BTN_ACTION_NAMES:    return "P";
    case BTN_ACTION_BIG_FOOD: return "F";
    case BTN_ACTION_HUD:      return "H";
    case BTN_ACTION_QUIT:     return "Q";
    default:                  return "-";
  }
}

void custom_controls_init_defaults(custom_controls_t* cc) {
  memset(cc, 0, sizeof(custom_controls_t));

  cc->show_joystick = true;
  cc->joystick_radius = 120.0f;
  cc->joystick_opacity = 0.5f;

  int idx = 0;

  // 1. Turbo / Boost (Botón principal inferior derecho)
  cc->buttons[idx] = (touch_button_t){
      .enabled = true,
      .name = "Turbo",
      .icon = "\ueaed",
      .action = BTN_ACTION_BOOST,
      .pos_x = 0.88f,
      .pos_y = 0.78f,
      .radius = 56.0f,
      .opacity = 0.85f,
      .color = 0x27AE60FF, // Verde Neón Esmeralda
      .is_down = false,
      .active_pointer_id = -1};
  idx++;

  // 2. Zoom In (Tecla N)
  cc->buttons[idx] = (touch_button_t){
      .enabled = true,
      .name = "Zoom +",
      .icon = "+",
      .action = BTN_ACTION_ZOOM_IN,
      .pos_x = 0.93f,
      .pos_y = 0.38f,
      .radius = 36.0f,
      .opacity = 0.75f,
      .color = 0x2980B9FF, // Azul
      .is_down = false,
      .active_pointer_id = -1};
  idx++;

  // 3. Zoom Out (Tecla M)
  cc->buttons[idx] = (touch_button_t){
      .enabled = true,
      .name = "Zoom -",
      .icon = "-",
      .action = BTN_ACTION_ZOOM_OUT,
      .pos_x = 0.93f,
      .pos_y = 0.52f,
      .radius = 36.0f,
      .opacity = 0.75f,
      .color = 0x2980B9FF, // Azul
      .is_down = false,
      .active_pointer_id = -1};
  idx++;

  // 4. Bot Defensivo (Tecla T)
  cc->buttons[idx] = (touch_button_t){
      .enabled = true,
      .name = "Bot",
      .icon = "\ue90c",
      .action = BTN_ACTION_BOT,
      .pos_x = 0.82f,
      .pos_y = 0.38f,
      .radius = 38.0f,
      .opacity = 0.80f,
      .color = 0x8E44ADFF, // Violeta
      .is_down = false,
      .active_pointer_id = -1};
  idx++;

  // 5. Asistencia (Tecla K)
  cc->buttons[idx] = (touch_button_t){
      .enabled = true,
      .name = "Asist",
      .icon = "\ue991",
      .action = BTN_ACTION_ASSIST,
      .pos_x = 0.82f,
      .pos_y = 0.52f,
      .radius = 38.0f,
      .opacity = 0.80f,
      .color = 0xD35400FF, // Naranja Ámbar
      .is_down = false,
      .active_pointer_id = -1};
  idx++;

  // 6. Reiniciar Partida (Tecla R)
  cc->buttons[idx] = (touch_button_t){
      .enabled = true,
      .name = "Reiniciar",
      .icon = "\ue9b6",
      .action = BTN_ACTION_RESTART,
      .pos_x = 0.93f,
      .pos_y = 0.12f,
      .radius = 34.0f,
      .opacity = 0.70f,
      .color = 0xC0392BFF, // Rojo
      .is_down = false,
      .active_pointer_id = -1};
  idx++;

  cc->button_count = idx;
}

void custom_controls_load(custom_controls_t* cc) {
  FILE* f = fopen(CUSTOM_CONTROLS_FILE, "rb");
  if (!f) {
    custom_controls_init_defaults(cc);
    custom_controls_save(cc);
    return;
  }

  size_t read_bytes = fread(cc, sizeof(custom_controls_t), 1, f);
  fclose(f);

  if (read_bytes != 1 || cc->button_count <= 0 || cc->button_count > MAX_TOUCH_BUTTONS) {
    custom_controls_init_defaults(cc);
    custom_controls_save(cc);
  } else {
    // Reset volatile runtime state
    for (int i = 0; i < cc->button_count; i++) {
      cc->buttons[i].is_down = false;
      cc->buttons[i].active_pointer_id = -1;
    }
  }
}

void custom_controls_save(const custom_controls_t* cc) {
  FILE* f = fopen(CUSTOM_CONTROLS_FILE, "wb");
  if (!f) return;
  fwrite(cc, sizeof(custom_controls_t), 1, f);
  fclose(f);
}

static void trigger_action_on_down(button_action_t act, tenv* env) {
  if (!env || !env->usr) return;
  tuser_data* usr = env->usr;
  game_data* gdata = &usr->gdata;
  user_settings* usrs = &usr->usrs;

  switch (act) {
    case BTN_ACTION_BOOST:
      gdata->bot.output.accel = true;
      break;
    case BTN_ACTION_ZOOM_IN:
      gdata->data.ms_zoom *= expf(1.0f * usrs->zoom_step);
      gdata->data.ms_zoom = GLM_MAX(MAX_ZOOM_OUT, GLM_MIN(gdata->data.ms_zoom, MAX_ZOOM_IN));
      break;
    case BTN_ACTION_ZOOM_OUT:
      gdata->data.ms_zoom *= expf(-1.0f * usrs->zoom_step);
      gdata->data.ms_zoom = GLM_MAX(MAX_ZOOM_OUT, GLM_MIN(gdata->data.ms_zoom, MAX_ZOOM_IN));
      break;
    case BTN_ACTION_BOT:
      usrs->hotkeys[HOTKEY_BOT].active ^= 1;
      break;
    case BTN_ACTION_ASSIST:
      usrs->hotkeys[HOTKEY_ASSIST].active ^= 1;
      break;
    case BTN_ACTION_RESTART:
      if (gdata->connection) {
        gdata->connection->is_closing = true;
        gdata->restart_req = true;
      }
      break;
    case BTN_ACTION_NAMES:
      usrs->hotkeys[HOTKEY_SHOW_NAMES].active ^= 1;
      break;
    case BTN_ACTION_BIG_FOOD:
      usrs->hotkeys[HOTKEY_BIG_FOOD].active ^= 1;
      break;
    case BTN_ACTION_HUD:
      usrs->hotkeys[HOTKEY_HUD].active ^= 1;
      break;
    case BTN_ACTION_QUIT:
      if (gdata->connection) {
        gdata->connection->is_closing = true;
      }
      gdata->conn = DISCONNECTED;
      break;
    default:
      break;
  }
}

static void trigger_action_on_up(button_action_t act, tenv* env) {
  if (!env || !env->usr) return;
  tuser_data* usr = env->usr;
  game_data* gdata = &usr->gdata;

  if (act == BTN_ACTION_BOOST) {
    gdata->bot.output.accel = false;
  }
}

bool custom_controls_touch_down(int pointer_id, float x, float y, float screen_w, float screen_h, tenv* env) {
  custom_controls_t* cc = &g_custom_controls;

  for (int i = 0; i < cc->button_count; i++) {
    touch_button_t* btn = &cc->buttons[i];
    if (!btn->enabled) continue;

    float bx = btn->pos_x * screen_w;
    float by = btn->pos_y * screen_h;
    float dx = x - bx;
    float dy = y - by;
    float r = btn->radius;

    if (dx * dx + dy * dy <= (r * 1.25f) * (r * 1.25f)) {
      btn->is_down = true;
      btn->active_pointer_id = pointer_id;
      trigger_action_on_down(btn->action, env);
      return true;
    }
  }
  return false;
}

bool custom_controls_touch_move(int pointer_id, float x, float y, float screen_w, float screen_h, tenv* env) {
  custom_controls_t* cc = &g_custom_controls;
  bool handled = false;

  for (int i = 0; i < cc->button_count; i++) {
    touch_button_t* btn = &cc->buttons[i];
    if (!btn->enabled) continue;

    if (btn->active_pointer_id == pointer_id) {
      float bx = btn->pos_x * screen_w;
      float by = btn->pos_y * screen_h;
      float dx = x - bx;
      float dy = y - by;
      float r = btn->radius * 1.6f;

      bool still_inside = (dx * dx + dy * dy <= r * r);
      if (btn->is_down != still_inside) {
        btn->is_down = still_inside;
        if (!still_inside) {
          trigger_action_on_up(btn->action, env);
        } else {
          trigger_action_on_down(btn->action, env);
        }
      }
      handled = true;
    }
  }
  return handled;
}

bool custom_controls_touch_up(int pointer_id, tenv* env) {
  custom_controls_t* cc = &g_custom_controls;
  bool handled = false;

  for (int i = 0; i < cc->button_count; i++) {
    touch_button_t* btn = &cc->buttons[i];
    if (!btn->enabled) continue;

    if (btn->active_pointer_id == pointer_id) {
      btn->is_down = false;
      btn->active_pointer_id = -1;
      trigger_action_on_up(btn->action, env);
      handled = true;
    }
  }
  return handled;
}

void custom_controls_reset_state(tenv* env) {
  custom_controls_t* cc = &g_custom_controls;
  for (int i = 0; i < cc->button_count; i++) {
    if (cc->buttons[i].is_down) {
      cc->buttons[i].is_down = false;
      cc->buttons[i].active_pointer_id = -1;
      trigger_action_on_up(cc->buttons[i].action, env);
    }
  }
}

bool custom_controls_is_boost_active(void) {
  custom_controls_t* cc = &g_custom_controls;
  for (int i = 0; i < cc->button_count; i++) {
    if (cc->buttons[i].enabled && cc->buttons[i].action == BTN_ACTION_BOOST && cc->buttons[i].is_down) {
      return true;
    }
  }
  return false;
}

void custom_controls_render_hud(tenv* env, float screen_w, float screen_h) {
  if (!env || !env->usr) return;
  tuser_data* usr = env->usr;
  user_settings* usrs = &usr->usrs;
  custom_controls_t* cc = &g_custom_controls;

  ImDrawList* dl = igGetForegroundDrawList_ViewportPtr(igGetMainViewport());
  if (!dl) return;

  for (int i = 0; i < cc->button_count; i++) {
    touch_button_t* btn = &cc->buttons[i];
    if (!btn->enabled) continue;

    float cx = btn->pos_x * screen_w;
    float cy = btn->pos_y * screen_h;
    float rad = btn->radius;

    // Check if this action is currently active as a toggle state
    bool is_toggled = false;
    if (btn->action == BTN_ACTION_BOT && usrs->hotkeys[HOTKEY_BOT].active) is_toggled = true;
    if (btn->action == BTN_ACTION_ASSIST && usrs->hotkeys[HOTKEY_ASSIST].active) is_toggled = true;
    if (btn->action == BTN_ACTION_NAMES && usrs->hotkeys[HOTKEY_SHOW_NAMES].active) is_toggled = true;
    if (btn->action == BTN_ACTION_BIG_FOOD && usrs->hotkeys[HOTKEY_BIG_FOOD].active) is_toggled = true;

    // Calculate colors
    float alpha = btn->opacity;
    if (btn->is_down) {
      rad += 4.0f; // Visual tactile expansion
      alpha = fminf(1.0f, alpha + 0.35f);
    } else if (is_toggled) {
      alpha = fminf(1.0f, alpha + 0.20f);
    }

    uint32_t r = (btn->color >> 24) & 0xFF;
    uint32_t g = (btn->color >> 16) & 0xFF;
    uint32_t b = (btn->color >> 8) & 0xFF;
    uint32_t a = (uint32_t)(alpha * 255.0f);

    uint32_t bg_col = (a / 2 << 24) | (b / 3 << 16) | (g / 3 << 8) | (r / 3);
    uint32_t border_col = (a << 24) | (b << 16) | (g << 8) | r;
    if (btn->is_down || is_toggled) {
      bg_col = (a << 24) | (b << 16) | (g << 8) | r;
      border_col = 0xFFFFFFFF;
    }

    // Outer glow ring
    ImDrawList_AddCircleFilled(dl, (ImVec2){cx, cy}, rad, bg_col, 36);
    ImDrawList_AddCircle(dl, (ImVec2){cx, cy}, rad, border_col, 36, btn->is_down ? 3.5f : 2.0f);

    // Text / Icon rendering
    ImFont* font = (rad > 46.0f)
                       ? usr->imgui_data.regular_font_bold[FONT_SIZE_LARGE]
                       : usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR];
    if (font) {
      const char* label = (btn->icon[0] != '\0') ? btn->icon : btn->name;
      ImVec2 txt_sz;
      ImFont_CalcTextSizeA(&txt_sz, font, font->LegacySize, FLT_MAX, -1.0f, label, NULL, NULL);
      ImVec2 txt_pos = {cx - txt_sz.x * 0.5f, cy - txt_sz.y * 0.5f};
      uint32_t txt_col = (btn->is_down || is_toggled) ? 0xFFFFFFFF : 0xE0FFFFFF;
      ImDrawList_AddText_FontPtr(dl, font, font->LegacySize, txt_pos, txt_col, label, NULL, 0.0f, NULL);
    }

    // Small subtitle under button if space permits and name differs from icon
    if (rad >= 42.0f && btn->icon[0] != '\0' && strcmp(btn->icon, btn->name) != 0) {
      ImFont* small_font = usr->imgui_data.regular_font[FONT_SIZE_SMALL];
      if (small_font) {
        ImVec2 sub_sz;
        ImFont_CalcTextSizeA(&sub_sz, small_font, small_font->LegacySize, FLT_MAX, -1.0f, btn->name, NULL, NULL);
        ImVec2 sub_pos = {cx - sub_sz.x * 0.5f, cy + rad * 0.35f};
        ImDrawList_AddText_FontPtr(dl, small_font, small_font->LegacySize, sub_pos, 0xCCFFFFFF, btn->name, NULL, 0.0f, NULL);
      }
    }
  }
}
