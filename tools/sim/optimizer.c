#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "simulator.h"

#define NUM_TRAIN_MATCHES 40
#define NUM_VAL_MATCHES 40
#define NUM_GENERATIONS 12
#define POPULATION_SIZE 8

typedef struct {
  sbot_weights_t weights;
  float fitness;
  float mean_survival;
  float mean_mass;
  int kills;
} candidate_t;

static float evaluate_weights(const sbot_weights_t* w, uint64_t base_seed, int num_matches,
                              float* out_time, float* out_mass, int* out_kills) {
  float sum_time = 0.0f;
  float sum_mass = 0.0f;
  int total_kills = 0;
  int early_deaths = 0;

  for (int m = 0; m < num_matches; m++) {
    sim_match_config_t cfg = {
      .seed = base_seed + (uint64_t)m * 17ULL,
      .max_duration_sec = 60.0f,
      .num_rivals = 8,
      .arena_radius = 2800.0f,
      .jsonl_log_path = NULL
    };

    sim_world_t world;
    sim_init(&world, &cfg, w);
    sim_match_result_t res = sim_run_match(&world);

    sum_time += res.survival_time;
    sum_mass += res.max_mass;
    total_kills += res.kills;
    if (res.survival_time < 8.0f) early_deaths++;
  }

  float mean_time = sum_time / (float)num_matches;
  float mean_mass = sum_mass / (float)num_matches;
  if (out_time) *out_time = mean_time;
  if (out_mass) *out_mass = mean_mass;
  if (out_kills) *out_kills = total_kills;

  float fitness = (mean_time * 1.5f) + (mean_mass * 0.7f) + (float)total_kills * 12.0f - (float)early_deaths * 25.0f;
  return fitness;
}

static void mutate_weights(sbot_weights_t* dst, const sbot_weights_t* src, uint64_t* rng, float sigma) {
  *dst = *src;

  // Perturb continuous parameters with gaussian-like uniform noise
  dst->safety_margin_base += sim_rng_float(rng, -1.0f, 1.0f) * (4.0f * sigma);
  if (dst->safety_margin_base < 18.0f) dst->safety_margin_base = 18.0f;
  if (dst->safety_margin_base > 45.0f) dst->safety_margin_base = 45.0f;

  dst->weight_clearance += sim_rng_float(rng, -1.0f, 1.0f) * (1.0f * sigma);
  if (dst->weight_clearance < 1.0f) dst->weight_clearance = 1.0f;

  dst->weight_food += sim_rng_float(rng, -1.0f, 1.0f) * (0.8f * sigma);
  if (dst->weight_food < 0.5f) dst->weight_food = 0.5f;

  dst->weight_border_repulse += sim_rng_float(rng, -1.0f, 1.0f) * (2.5f * sigma);
  if (dst->weight_border_repulse < 5.0f) dst->weight_border_repulse = 5.0f;

  dst->escape_enter_dist += sim_rng_float(rng, -1.0f, 1.0f) * (20.0f * sigma);
  if (dst->escape_enter_dist < 150.0f) dst->escape_enter_dist = 150.0f;
  if (dst->escape_enter_dist > 350.0f) dst->escape_enter_dist = 350.0f;

  dst->escape_exit_dist = dst->escape_enter_dist + sim_rng_float(rng, 150.0f, 250.0f);
}

int main(void) {
  printf("=========================================================================\n");
  printf("   YSLITHER PHASE 4 PARAMETER OPTIMIZER (CMA-ES / EVOLUTIONARY STRATEGY) \n");
  printf("   Generations: %d | Population: %d | Train Seeds != Val Seeds            \n",
         NUM_GENERATIONS, POPULATION_SIZE);
  printf("=========================================================================\n");

  uint64_t rng = 987654321ULL;
  candidate_t best_global;
  sbot_weights_init_default(&best_global.weights);

  uint64_t train_base_seed = 1000000ULL;
  uint64_t val_base_seed   = 9000000ULL;

  best_global.fitness = evaluate_weights(&best_global.weights, train_base_seed, NUM_TRAIN_MATCHES,
                                         &best_global.mean_survival, &best_global.mean_mass, &best_global.kills);

  printf("Generation 0 (Baseline): Fitness=%.2f | Supervivencia=%.2fs | Masa=%.1f | Kills=%d\n",
         best_global.fitness, best_global.mean_survival, best_global.mean_mass, best_global.kills);

  float sigma = 1.0f;

  for (int gen = 1; gen <= NUM_GENERATIONS; gen++) {
    candidate_t pop[POPULATION_SIZE];
    int best_pop_idx = -1;
    float best_pop_fitness = -1e9f;

    for (int p = 0; p < POPULATION_SIZE; p++) {
      mutate_weights(&pop[p].weights, &best_global.weights, &rng, sigma);
      pop[p].fitness = evaluate_weights(&pop[p].weights, train_base_seed + (uint64_t)gen * 1000ULL,
                                        NUM_TRAIN_MATCHES, &pop[p].mean_survival, &pop[p].mean_mass, &pop[p].kills);

      if (pop[p].fitness > best_pop_fitness) {
        best_pop_fitness = pop[p].fitness;
        best_pop_idx = p;
      }
    }

    if (best_pop_fitness > best_global.fitness) {
      best_global = pop[best_pop_idx];
      printf("Gen %2d [MEJORA]: Fitness=%.2f | Supervivencia=%.2fs | Masa=%.1f | Kills=%d (sigma=%.2f)\n",
             gen, best_global.fitness, best_global.mean_survival, best_global.mean_mass, best_global.kills, sigma);
      sigma = fminf(1.5f, sigma * 1.05f);
    } else {
      printf("Gen %2d [ESTABLE]: Mejor=%.2f vs Global=%.2f\n", gen, best_pop_fitness, best_global.fitness);
      sigma = fmaxf(0.2f, sigma * 0.90f); // 1/5th success rule cooling
    }
  }

  // Final Validation on Unseen Seeds
  printf("\n=== Realizando Validación en Semillas Independientes (Evitar Sobreajuste) ===\n");
  float val_time, val_mass;
  int val_kills;
  float val_fit = evaluate_weights(&best_global.weights, val_base_seed, NUM_VAL_MATCHES,
                                   &val_time, &val_mass, &val_kills);

  printf("Resultado de Validación Final:\n");
  printf("  Fitness Validación  : %.2f\n", val_fit);
  printf("  Supervivencia Media : %.2f s\n", val_time);
  printf("  Masa Máxima Media   : %.1f\n", val_mass);
  printf("  Kills Totales       : %d\n", val_kills);

  printf("\nParámetros Optimizados:\n");
  printf("  safety_margin_base   = %.2f\n", best_global.weights.safety_margin_base);
  printf("  weight_clearance     = %.2f\n", best_global.weights.weight_clearance);
  printf("  weight_food          = %.2f\n", best_global.weights.weight_food);
  printf("  weight_border_repulse= %.2f\n", best_global.weights.weight_border_repulse);
  printf("  escape_enter_dist    = %.2f\n", best_global.weights.escape_enter_dist);
  printf("  escape_exit_dist     = %.2f\n", best_global.weights.escape_exit_dist);

  return 0;
}
