#ifndef SBOT_WEIGHTS_H
#define SBOT_WEIGHTS_H

#include <stdbool.h>

// sbot_weights_t: Parameterized bot configuration for kinematic evaluation,
// hysteresis mode transitions, and online adaptive risk learning.
typedef struct {
  // Perception & Kinematic Horizon
  float safety_margin_base;     // Margen de seguridad base sobre el radio de colision (px)
  float safety_margin_scale;    // Factor de escala de seguridad segun velocidad
  float horizon_time;           // Tiempo total de proyección cinemática (s)
  int rollout_substeps;         // Cantidad de pasos de integración por rollout (4 o 5)

  // Scoring Weights
  float weight_clearance;       // Recompensa por espacio libre alcanzable / corredor despejado
  float weight_food;            // Atracción hacia comida ponderada por masa y distancia
  float weight_risk_body;       // Penalización por proximidad a cuerpos de serpientes
  float weight_risk_head;       // Penalización severa por cabezas rivales grandes
  float weight_hunt_cut;        // Recompensa por cortar el paso a cabezas rivales (kill opportunity)
  float weight_border_repulse;  // Fuerza repulsiva ante el borde del mapa
  float weight_turn_penalty;    // Penalización por giros bruscos (inercia direccional)
  float weight_boost_cost;      // Penalización por consumo de masa en turbo

  // Mode Thresholds & Hysteresis
  float escape_enter_dist;      // Distancia frontal a obstáculo para activar ESCAPE (px)
  float escape_exit_dist;       // Distancia de despeje para salir de ESCAPE (px)
  int escape_min_frames;        // Mínimo de fotogramas despejados antes de abandonar ESCAPE
  float hunt_min_advantage;     // Ventaja de masa requerida para entrar en modo HUNT
  float hunt_max_range;         // Alcance máximo para fijar objetivo en HUNT (px)

  // Smooth Steering & Kinematics
  float angle_smooth_rate;      // Tasa de convergencia exponencial del ángulo (rad/s)
  float max_turn_rate_base;     // Velocidad angular máxima base de giro (rad/s)

  // Online Adaptive Risk Learning
  float adaptive_safety_delta;  // Desviación acumulada por historial reciente de muertes (px)
} sbot_weights_t;

// Default highly-tuned baseline weights
static inline void sbot_weights_init_default(sbot_weights_t* w) {
  w->safety_margin_base = 32.0f;
  w->safety_margin_scale = 0.20f;
  w->horizon_time = 0.95f;
  w->rollout_substeps = 4;

  w->weight_clearance = 5.0f;
  w->weight_food = 2.4f;
  w->weight_risk_body = 9.0f;
  w->weight_risk_head = 18.0f;
  w->weight_hunt_cut = 7.5f;
  w->weight_border_repulse = 15.0f;
  w->weight_turn_penalty = 1.3f;
  w->weight_boost_cost = 2.8f;

  w->escape_enter_dist = 230.0f;
  w->escape_exit_dist = 440.0f;
  w->escape_min_frames = 15;
  w->hunt_min_advantage = 0.95f;
  w->hunt_max_range = 560.0f;

  w->angle_smooth_rate = 24.0f;
  w->max_turn_rate_base = 5.2f;

  w->adaptive_safety_delta = 0.0f;
}

// Adaptación online ligera: amplía o relaja el margen de seguridad de forma segura
static inline void sbot_weights_record_death_event(sbot_weights_t* w, int death_cause) {
  if (!w) return;
  // death_cause: 1 = choque de cabeza contra cuerpo, 2 = cabeza contra cabeza rival, 3 = borde del mapa
  if (death_cause == 1) {
    w->adaptive_safety_delta += 3.5f; // Mayor precaución ante cuerpos
  } else if (death_cause == 2) {
    w->adaptive_safety_delta += 5.0f; // Mayor margen ante cabezas rivales
  } else if (death_cause == 3) {
    w->weight_border_repulse += 2.0f; // Más fuerza de alejamiento del borde
  }
  // Clamp a rangos seguros de estabilidad
  if (w->adaptive_safety_delta > 25.0f) w->adaptive_safety_delta = 25.0f;
  if (w->adaptive_safety_delta < -10.0f) w->adaptive_safety_delta = -10.0f;
  if (w->weight_border_repulse > 30.0f) w->weight_border_repulse = 30.0f;
}

#endif // SBOT_WEIGHTS_H
