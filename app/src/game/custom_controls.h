#ifndef CUSTOM_CONTROLS_H
#define CUSTOM_CONTROLS_H

#include <stdbool.h>
#include <stdint.h>

struct tenv;
typedef struct tenv tenv;

typedef enum {
  BTN_ACTION_BOOST = 0,     // Acelerar / Turbo (Space)
  BTN_ACTION_ZOOM_IN,       // Acercar cámara (Tecla N)
  BTN_ACTION_ZOOM_OUT,      // Alejar cámara (Tecla M)
  BTN_ACTION_BOT,           // Activar/Desactivar Bot Defensivo (Tecla T)
  BTN_ACTION_ASSIST,        // Modo Asistencia (Tecla K)
  BTN_ACTION_RESTART,       // Reiniciar partida (Tecla R)
  BTN_ACTION_NAMES,         // Mostrar/Ocultar nombres (Tecla P)
  BTN_ACTION_BIG_FOOD,      // Alternar tamaño de comida (Tecla F)
  BTN_ACTION_HUD,           // Alternar HUD (Tecla H)
  BTN_ACTION_QUIT,          // Salir al menú (Tecla Q)
  BTN_ACTION_FEEDER,        // Alternar Bots Alimentadores (Comida)
  BTN_ACTION_COUNT
} button_action_t;

typedef struct {
  bool enabled;
  char name[24];            // Nombre amigable ("Turbo", "Zoom +", "Bot", etc.)
  char icon[16];           // Glifo UTF-8 o texto representativo
  button_action_t action;   // Acción a ejecutar
  float pos_x;              // Posición horizontal normalizada (0.0 = izquierda, 1.0 = derecha)
  float pos_y;              // Posición vertical normalizada (0.0 = arriba, 1.0 = abajo)
  float radius;             // Radio del botón táctil en píxeles (30px a 90px)
  float opacity;            // Opacidad visual (0.2 a 1.0)
  uint32_t color;           // Color temático RGBA (0xRRGGBBAA)
  bool is_down;             // Estado presionado en tiempo real
  int active_pointer_id;    // ID del puntero multitáctil activo (-1 si ninguno)
} touch_button_t;

#define MAX_TOUCH_BUTTONS 10

typedef struct {
  int button_count;
  touch_button_t buttons[MAX_TOUCH_BUTTONS];
  bool show_joystick;
  float joystick_radius;
  float joystick_opacity;
} custom_controls_t;

extern custom_controls_t g_custom_controls;

// Inicialización y persistencia
void custom_controls_init_defaults(custom_controls_t* cc);
void custom_controls_load(custom_controls_t* cc);
void custom_controls_save(const custom_controls_t* cc);

// Detección táctil en tiempo real
bool custom_controls_touch_down(int pointer_id, float x, float y, float screen_w, float screen_h, tenv* env);
bool custom_controls_touch_move(int pointer_id, float x, float y, float screen_w, float screen_h, tenv* env);
bool custom_controls_touch_up(int pointer_id, tenv* env);
void custom_controls_reset_state(tenv* env);
bool custom_controls_is_boost_active(void);

// Renderizado en el HUD durante la partida
void custom_controls_render_hud(tenv* env, float screen_w, float screen_h);

// Nombres descriptivos de acciones
const char* custom_controls_action_name(button_action_t action);
const char* custom_controls_action_key_str(button_action_t action);

#endif
