#include "controls_editor.h"

#include <stdio.h>
#include <string.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "../cimgui/cimgui.h"
#include "../game/custom_controls.h"
#include "../user.h"

static int s_selected_idx = 0;
static bool s_initialized = false;
static custom_controls_t s_edit_copy;

void ui_controls_editor_init(tenv* env) {
  (void)env;
  custom_controls_load(&g_custom_controls);
  memcpy(&s_edit_copy, &g_custom_controls, sizeof(custom_controls_t));
  s_selected_idx = 0;
  s_initialized = true;
}

void ui_controls_editor(tenv* env) {
  if (!s_initialized) {
    ui_controls_editor_init(env);
  }

  tuser_data* usr = env->usr;
  tcontext* ctx = env->ctx;
  ImGuiStyle* style = igGetStyle();
  float screen_w = (float)ctx->size[0];
  float screen_h = (float)ctx->size[1];

  // Header Title
  igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_LARGE],
             usr->imgui_data.regular_font_bold[FONT_SIZE_LARGE]->LegacySize);
  igSetCursorPos((ImVec2){24.0f, 16.0f});
  igTextColored((ImVec4){0.25f, 0.85f, 0.50f, 1.0f}, "\ue991  MODIFICADOR DE CONTROLES TACTILES");
  igPopFont();

  igPushFont(usr->imgui_data.regular_font[FONT_SIZE_SMALL],
             usr->imgui_data.regular_font[FONT_SIZE_SMALL]->LegacySize);
  igSetCursorPos((ImVec2){24.0f, 50.0f});
  igTextColored((ImVec4){0.70f, 0.75f, 0.80f, 0.90f},
                "Personaliza el tamano, posicion, icono y accion de cada boton en pantalla.");
  igPopFont();

  float top_h = 76.0f;
  float bottom_bar_h = 60.0f;
  float content_h = screen_h - top_h - bottom_bar_h;
  float left_panel_w = fminf(480.0f, screen_w * 0.44f);
  float right_canvas_w = screen_w - left_panel_w - 36.0f;

  igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 8.0f);
  igPushStyleVar_Vec2(ImGuiStyleVar_ItemSpacing, (ImVec2){8.0f, 8.0f});

  // LEFT PANEL: Controls List & Property Inspector
  igSetCursorPos((ImVec2){18.0f, top_h});
  igPushStyleColor_Vec4(ImGuiCol_ChildBg, (ImVec4){0.11f, 0.13f, 0.17f, 0.95f});
  igPushStyleColor_Vec4(ImGuiCol_Border, (ImVec4){0.20f, 0.28f, 0.38f, 0.80f});
  igPushStyleVar_Float(ImGuiStyleVar_ChildRounding, 12.0f);
  igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 1.5f);

  if (igBeginChild_Str("left_panel", (ImVec2){left_panel_w, content_h}, true, ImGuiWindowFlags_None)) {
    igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
               usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
    igTextColored((ImVec4){0.35f, 0.70f, 1.0f, 1.0f}, "Lista de Botones");
    igPopFont();

    // Buttons list
    float list_h = fminf(170.0f, content_h * 0.35f);
    if (igBeginChild_Str("buttons_list_box", (ImVec2){-1, list_h}, true, ImGuiWindowFlags_None)) {
      for (int i = 0; i < s_edit_copy.button_count; i++) {
        touch_button_t* btn = &s_edit_copy.buttons[i];
        igPushID_Int(i);

        bool selected = (i == s_selected_idx);
        if (selected) {
          igPushStyleColor_Vec4(ImGuiCol_Header, (ImVec4){0.20f, 0.45f, 0.65f, 0.85f});
        }

        // Active Checkbox
        igCheckbox("##btn_active", &btn->enabled);
        igSameLine(0, -1);

        char item_label[64];
        snprintf(item_label, sizeof(item_label), "%s %s (%s)",
                 btn->icon[0] != '\0' ? btn->icon : "",
                 btn->name,
                 custom_controls_action_key_str(btn->action));

        if (igSelectable_Bool(item_label, selected, ImGuiSelectableFlags_None, (ImVec2){-1, 26})) {
          s_selected_idx = i;
        }

        if (selected) {
          igPopStyleColor(1);
        }
        igPopID();
      }
    }
    igEndChild();

    // Action buttons: Agregar, Eliminar, Restablecer
    float col_w = (left_panel_w - 40.0f) / 3.0f;
    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.18f, 0.55f, 0.32f, 1.0f});
    if (igButton("+ Agregar", (ImVec2){col_w, 36.0f})) {
      if (s_edit_copy.button_count < MAX_TOUCH_BUTTONS) {
        int new_idx = s_edit_copy.button_count;
        s_edit_copy.buttons[new_idx] = (touch_button_t){
            .enabled = true,
            .name = "Boton N",
            .icon = "N",
            .action = BTN_ACTION_ZOOM_IN,
            .pos_x = 0.85f,
            .pos_y = 0.25f,
            .radius = 38.0f,
            .opacity = 0.80f,
            .color = 0x27AE60FF,
            .is_down = false,
            .active_pointer_id = -1};
        s_edit_copy.button_count++;
        s_selected_idx = new_idx;
      }
    }
    igPopStyleColor(1);
    igSameLine(0, -1);

    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.60f, 0.22f, 0.22f, 1.0f});
    if (igButton("Eliminar", (ImVec2){col_w, 36.0f})) {
      if (s_edit_copy.button_count > 1 && s_selected_idx >= 0 && s_selected_idx < s_edit_copy.button_count) {
        for (int j = s_selected_idx; j < s_edit_copy.button_count - 1; j++) {
          s_edit_copy.buttons[j] = s_edit_copy.buttons[j + 1];
        }
        s_edit_copy.button_count--;
        if (s_selected_idx >= s_edit_copy.button_count) s_selected_idx = s_edit_copy.button_count - 1;
      }
    }
    igPopStyleColor(1);
    igSameLine(0, -1);

    if (igButton("Reset", (ImVec2){col_w, 36.0f})) {
      custom_controls_init_defaults(&s_edit_copy);
      s_selected_idx = 0;
    }

    igSeparator();
    igSpacing();

    // INSPECTOR FOR SELECTED BUTTON
    if (s_selected_idx >= 0 && s_selected_idx < s_edit_copy.button_count) {
      touch_button_t* sel = &s_edit_copy.buttons[s_selected_idx];

      igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
                 usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
      igTextColored((ImVec4){0.20f, 0.90f, 0.50f, 1.0f}, "Propiedades: %s", sel->name);
      igPopFont();

      // 1. Accion / Tecla
      igText("Accion / Tecla:");
      int current_act = (int)sel->action;
      const char* action_items[] = {
          "Turbo / Acelerar (Espacio)",
          "Acercar Zoom (Tecla N)",
          "Alejar Zoom (Tecla M)",
          "Bot Defensivo (Tecla T)",
          "Modo Asistencia (Tecla K)",
          "Reiniciar Partida (Tecla R)",
          "Mostrar Nombres (Tecla P)",
          "Comida Grande (Tecla F)",
          "Alternar HUD (Tecla H)",
          "Salir al Menu (Tecla Q)"};
      igSetNextItemWidth(-1);
      if (igCombo_Str_arr("##action_combo", &current_act, action_items, 10, -1)) {
        sel->action = (button_action_t)current_act;
        // Auto-assign sensible default icon and label if matching
        if (sel->action == BTN_ACTION_BOOST) {
          strcpy(sel->name, "Turbo");
          strcpy(sel->icon, "\ueaed");
        } else if (sel->action == BTN_ACTION_ZOOM_IN) {
          strcpy(sel->name, "Zoom +");
          strcpy(sel->icon, "+");
        } else if (sel->action == BTN_ACTION_ZOOM_OUT) {
          strcpy(sel->name, "Zoom -");
          strcpy(sel->icon, "-");
        } else if (sel->action == BTN_ACTION_BOT) {
          strcpy(sel->name, "Bot");
          strcpy(sel->icon, "\ue90c");
        } else if (sel->action == BTN_ACTION_ASSIST) {
          strcpy(sel->name, "Asist");
          strcpy(sel->icon, "\ue991");
        } else if (sel->action == BTN_ACTION_RESTART) {
          strcpy(sel->name, "Reiniciar");
          strcpy(sel->icon, "\ue9b6");
        }
      }

      // 2. Icono Selector
      igSpacing();
      igText("Icono:");
      const char* icons[] = {"\ueaed", "\ue90c", "+", "-", "\ue991", "\ue9b6", "\ue99e", "\uea1c", "N", "M", "T", "K", "R", "P"};
      const int num_icons = sizeof(icons) / sizeof(icons[0]);
      for (int k = 0; k < num_icons; k++) {
        bool is_curr = (strcmp(sel->icon, icons[k]) == 0);
        if (is_curr) {
          igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.20f, 0.70f, 0.40f, 1.0f});
        }
        if (igButton(icons[k], (ImVec2){36.0f, 32.0f})) {
          strncpy(sel->icon, icons[k], sizeof(sel->icon));
        }
        if (is_curr) igPopStyleColor(1);
        if ((k + 1) % 7 != 0 && k < num_icons - 1) igSameLine(0, -1);
      }

      // 3. Tamano / Radio
      igSpacing();
      igText("Tamano (Radio): %.0f px", sel->radius);
      igSetNextItemWidth(left_panel_w - 120.0f);
      igSliderFloat("##btn_radius", &sel->radius, 26.0f, 85.0f, "%.0f px", ImGuiSliderFlags_None);
      igSameLine(0, -1);
      if (igButton("-##r_dec", (ImVec2){36.0f, 26.0f})) {
        sel->radius = fmaxf(26.0f, sel->radius - 4.0f);
      }
      igSameLine(0, -1);
      if (igButton("+##r_inc", (ImVec2){36.0f, 26.0f})) {
        sel->radius = fminf(85.0f, sel->radius + 4.0f);
      }

      // 4. Posicion Horizontal (X)
      igSpacing();
      igText("Posicion Horizontal (X): %.0f%%", sel->pos_x * 100.0f);
      igSetNextItemWidth(left_panel_w - 120.0f);
      igSliderFloat("##pos_x", &sel->pos_x, 0.05f, 0.95f, "%.2f", ImGuiSliderFlags_None);
      igSameLine(0, -1);
      if (igButton("<##x_left", (ImVec2){36.0f, 26.0f})) {
        sel->pos_x = fmaxf(0.05f, sel->pos_x - 0.03f);
      }
      igSameLine(0, -1);
      if (igButton(">##x_right", (ImVec2){36.0f, 26.0f})) {
        sel->pos_x = fminf(0.95f, sel->pos_x + 0.03f);
      }

      // 5. Posicion Vertical (Y)
      igSpacing();
      igText("Posicion Vertical (Y): %.0f%%", sel->pos_y * 100.0f);
      igSetNextItemWidth(left_panel_w - 120.0f);
      igSliderFloat("##pos_y", &sel->pos_y, 0.06f, 0.94f, "%.2f", ImGuiSliderFlags_None);
      igSameLine(0, -1);
      if (igButton("^##y_up", (ImVec2){36.0f, 26.0f})) {
        sel->pos_y = fmaxf(0.06f, sel->pos_y - 0.03f);
      }
      igSameLine(0, -1);
      if (igButton("v##y_down", (ImVec2){36.0f, 26.0f})) {
        sel->pos_y = fminf(0.94f, sel->pos_y + 0.03f);
      }

      // 6. Opacidad
      igSpacing();
      igText("Transparencia: %.0f%%", sel->opacity * 100.0f);
      igSetNextItemWidth(-1);
      igSliderFloat("##btn_opacity", &sel->opacity, 0.20f, 1.00f, "%.2f", ImGuiSliderFlags_None);

      // 7. Color Tematico
      igSpacing();
      igText("Color:");
      uint32_t palette[] = {0x27AE60FF, 0x2980B9FF, 0x8E44ADFF, 0xD35400FF, 0xC0392BFF, 0xF39C12FF, 0x16A085FF, 0x7F8C8DFF};
      for (int p = 0; p < 8; p++) {
        uint32_t c = palette[p];
        float rf = ((c >> 24) & 0xFF) / 255.0f;
        float gf = ((c >> 16) & 0xFF) / 255.0f;
        float bf = ((c >> 8) & 0xFF) / 255.0f;
        igPushID_Int(p + 100);
        igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){rf, gf, bf, 1.0f});
        if (igButton("##color_swatch", (ImVec2){34.0f, 26.0f})) {
          sel->color = c;
        }
        igPopStyleColor(1);
        igPopID();
        if (p < 7) igSameLine(0, -1);
      }
    }
  }
  igEndChild();
  igPopStyleVar(2);
  igPopStyleColor(2);

  // RIGHT CANVAS: Interactive Live Game Screen Preview
  float canvas_x = left_panel_w + 30.0f;
  igSetCursorPos((ImVec2){canvas_x, top_h});
  igPushStyleColor_Vec4(ImGuiCol_ChildBg, (ImVec4){0.07f, 0.09f, 0.12f, 1.0f});
  igPushStyleColor_Vec4(ImGuiCol_Border, (ImVec4){0.20f, 0.70f, 0.40f, 0.60f});
  igPushStyleVar_Float(ImGuiStyleVar_ChildRounding, 12.0f);
  igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 2.0f);

  if (igBeginChild_Str("preview_canvas", (ImVec2){right_canvas_w, content_h}, true, ImGuiWindowFlags_None)) {
    ImDrawList* dl = igGetWindowDrawList();
    ImVec2 canvas_p0;
    igGetCursorScreenPos(&canvas_p0);
    float cw = right_canvas_w - style->WindowPadding.x * 2;
    float ch = content_h - style->WindowPadding.y * 2;

    // Background preview grid
    uint32_t grid_col = 0x18FFFFFF;
    for (float gx = 0; gx < cw; gx += 40.0f) {
      ImDrawList_AddLine(dl, (ImVec2){canvas_p0.x + gx, canvas_p0.y},
                         (ImVec2){canvas_p0.x + gx, canvas_p0.y + ch}, grid_col, 1.0f);
    }
    for (float gy = 0; gy < ch; gy += 40.0f) {
      ImDrawList_AddLine(dl, (ImVec2){canvas_p0.x, canvas_p0.y + gy},
                         (ImVec2){canvas_p0.x + cw, canvas_p0.y + gy}, grid_col, 1.0f);
    }

    // Left side touch area indicator (Joystick zone)
    ImDrawList_AddRectFilled(dl, canvas_p0, (ImVec2){canvas_p0.x + cw * 0.55f, canvas_p0.y + ch}, 0x0C27AE60, 8.0f, ImDrawFlags_None);
    ImFont* small_font = usr->imgui_data.regular_font[FONT_SIZE_SMALL];
    if (small_font) {
      ImDrawList_AddText_FontPtr(dl, small_font, small_font->LegacySize,
                                 (ImVec2){canvas_p0.x + 16.0f, canvas_p0.y + ch - 30.0f},
                                 0x80FFFFFF, "Zona de Control de Movimiento (Joystick)", NULL, 0.0f, NULL);
    }

    // Draw all active buttons in the preview
    for (int i = 0; i < s_edit_copy.button_count; i++) {
      touch_button_t* btn = &s_edit_copy.buttons[i];
      if (!btn->enabled) continue;

      float bx = canvas_p0.x + btn->pos_x * cw;
      float by = canvas_p0.y + btn->pos_y * ch;
      float br = btn->radius * (cw / screen_w); // scale radius to preview size
      if (br < 18.0f) br = 18.0f;

      bool is_sel = (i == s_selected_idx);

      uint32_t r = (btn->color >> 24) & 0xFF;
      uint32_t g = (btn->color >> 16) & 0xFF;
      uint32_t b = (btn->color >> 8) & 0xFF;
      uint32_t a = (uint32_t)(btn->opacity * 255.0f);

      uint32_t fill_col = (a / 2 << 24) | (b / 2 << 16) | (g / 2 << 8) | (r / 2);
      uint32_t border_col = is_sel ? 0xFFFFFFFF : ((a << 24) | (b << 16) | (g << 8) | r);

      ImDrawList_AddCircleFilled(dl, (ImVec2){bx, by}, br, fill_col, 32);
      ImDrawList_AddCircle(dl, (ImVec2){bx, by}, br, border_col, 32, is_sel ? 3.5f : 1.8f);

      if (is_sel) {
        // Selection highlight ring
        ImDrawList_AddCircle(dl, (ImVec2){bx, by}, br + 6.0f, 0x9027AE60, 32, 2.0f);
      }

      // Draw button icon/name
      ImFont* font = (br > 26.0f) ? usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]
                                  : usr->imgui_data.regular_font[FONT_SIZE_SMALL];
      if (font) {
        const char* lbl = (btn->icon[0] != '\0') ? btn->icon : btn->name;
        ImVec2 tsz;
        ImFont_CalcTextSizeA(&tsz, font, font->LegacySize, FLT_MAX, -1.0f, lbl, NULL, NULL);
        ImDrawList_AddText_FontPtr(dl, font, font->LegacySize,
                                   (ImVec2){bx - tsz.x * 0.5f, by - tsz.y * 0.5f},
                                   0xFFFFFFFF, lbl, NULL, 0.0f, NULL);
      }
    }
  }
  igEndChild();
  igPopStyleVar(2);
  igPopStyleColor(2);

  // BOTTOM ACTION BAR: Guardar y Salir / Cancelar
  float bar_y = screen_h - bottom_bar_h;
  igSetCursorPos((ImVec2){18.0f, bar_y + 4.0f});

  igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
             usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);

  float btn_save_w = 260.0f;
  float btn_cancel_w = 160.0f;

  igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.14f, 0.72f, 0.35f, 1.0f});
  igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.18f, 0.85f, 0.42f, 1.0f});
  if (igButton("\uea1c  Guardar y Salir", (ImVec2){btn_save_w, 48.0f})) {
    memcpy(&g_custom_controls, &s_edit_copy, sizeof(custom_controls_t));
    custom_controls_save(&g_custom_controls);
    usr->gdata.curr_screen = TITLE_SCREEN;
  }
  igPopStyleColor(2);
  igSameLine(0, -1);

  igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.25f, 0.28f, 0.35f, 1.0f});
  if (igButton("Cancelar", (ImVec2){btn_cancel_w, 48.0f})) {
    usr->gdata.curr_screen = TITLE_SCREEN;
  }
  igPopStyleColor(1);
  igPopFont();

  igPopStyleVar(2);
}
