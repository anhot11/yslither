#ifndef USER_SETTINGS_H
#define USER_SETTINGS_H

#include <cglm/cglm.h>
#include <stdbool.h>
#include <stdint.h>

#include "../constants.h"

#ifndef SETTINGS_VERSION
#define SETTINGS_VERSION "1.3"
#endif

typedef struct hotkey {
  int key;
  bool active;
  int mode;
  char description[MAX_HOTKEY_DESC_LENGTH + 1];
} hotkey;

typedef struct gameplay_mode {
  bool food_flicker;
  bool food_float;
  bool uniform_food_color;
  bool show_crosshair;
  bool show_boost;
  bool show_shadows;
  bool show_background;
  bool show_accessories;
  bool death_effect;
  bool player_names_outline;
  bool const_food_scale;
  int food_type;
  int boost_type;
  int render_mode;
  float food_scale;
  float qsm;
  float bg_scale;
  float boost_strength;
  vec3 food_color;
} gameplay_mode;

typedef struct user_settings {
  char version[4];
  char nickname[MAX_NICKNAME_LEN + 1];
  char ipv4[MAX_IPV4_LEN + 1];
  char skin_code[MAX_SKIN_CODE_LEN + 1];
  uint8_t accessory;
  bool custom_skin;
  uint8_t default_skin;
  int score;
  double play_time;
  int kills;
  font_size ui_font_size;
  font_size lb_font_size;
  font_size snake_names_font_size;
  font_size stats_font_size;

  // global settings:
  vec3 bd_color;
  vec4 laser_color;
  int laser_thickness;
  int cursor_size;
  int minimap_size;
  bool restart_rc;
  bool quit_mc;
  bool vsync;
  bool smooth_zoom;
  bool snake_scores;
  bool instant_restart;
  float zoom_step;
  int bot_radius_mult;
  int bot_follow_circle_score;
  int bot_mode;              // 0 = Ultra-Defensivo, 1 = Caza / Equilibrado, 2 = Auto-Coil Continuo
  bool bot_visual_line;      // Ver linea del bot hacia el objetivo
  bool bot_visual_zones;     // Ver zonas rojas de peligro y colision
  bool bot_visual_radar;     // Ver radar de sensores y circulos de esquiva
  bool bot_visual_food;      // Ver linea verde hacia el objetivo de comida
  bool bot_auto_turbo;       // Permitir turbo inteligente en situaciones seguras
  bool bot_auto_start;       // Activar el bot automaticamente al iniciar partida
  bool debug_logs_enabled;   // Modo Debug & Registro de Telemetria/Logs (default: true)

  gameplay_mode modes[2];

  // hotkeys:
  hotkey hotkeys[NUM_HOTKEYS];
} user_settings;

void user_settings_default(user_settings* usr_settings);
void read_user_settings(user_settings* usr_settings);
void save_user_settings(user_settings* usr_settings);

#endif