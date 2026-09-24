#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "simulator.h"

#define NUM_BENCHMARK_MATCHES 200

typedef struct {
  float survival_times[NUM_BENCHMARK_MATCHES];
  float max_masses[NUM_BENCHMARK_MATCHES];
  int kills[NUM_BENCHMARK_MATCHES];
  int death_causes[NUM_BENCHMARK_MATCHES];

  float mean_survival;
  float median_survival;
  float std_survival;
  float mean_max_mass;
  float median_max_mass;
  float total_kills;
  float mean_kills;

  int deaths_body;
  int deaths_head;
  int deaths_border;
  int survived_full;
} bench_stats_t;

static int float_cmp(const void* a, const void* b) {
  float fa = *(const float*)a;
  float fb = *(const float*)b;
  return (fa > fb) - (fa < fb);
}

static void compute_stats(bench_stats_t* s, int count) {
  float sum_time = 0.0f;
  float sum_mass = 0.0f;
  s->total_kills = 0;
  s->deaths_body = 0;
  s->deaths_head = 0;
  s->deaths_border = 0;
  s->survived_full = 0;

  float sorted_times[NUM_BENCHMARK_MATCHES];
  float sorted_mass[NUM_BENCHMARK_MATCHES];

  for (int i = 0; i < count; i++) {
    sum_time += s->survival_times[i];
    sum_mass += s->max_masses[i];
    s->total_kills += s->kills[i];
    sorted_times[i] = s->survival_times[i];
    sorted_mass[i] = s->max_masses[i];

    if (s->death_causes[i] == DEATH_CAUSE_BODY) s->deaths_body++;
    else if (s->death_causes[i] == DEATH_CAUSE_HEAD) s->deaths_head++;
    else if (s->death_causes[i] == DEATH_CAUSE_BORDER) s->deaths_border++;
    else s->survived_full++;
  }

  s->mean_survival = sum_time / (float)count;
  s->mean_max_mass = sum_mass / (float)count;
  s->mean_kills = (float)s->total_kills / (float)count;

  qsort(sorted_times, count, sizeof(float), float_cmp);
  qsort(sorted_mass, count, sizeof(float), float_cmp);

  s->median_survival = sorted_times[count / 2];
  s->median_max_mass = sorted_mass[count / 2];

  // Standard deviation
  float var_sum = 0.0f;
  for (int i = 0; i < count; i++) {
    float diff = s->survival_times[i] - s->mean_survival;
    var_sum += diff * diff;
  }
  s->std_survival = sqrtf(var_sum / (float)count);
}

static void run_eval_batch(const char* label, const sbot_weights_t* weights,
                           const char* jsonl_path, bench_stats_t* out_stats) {
  memset(out_stats, 0, sizeof(bench_stats_t));

  if (jsonl_path) {
    // Clear/create file
    FILE* fp = fopen(jsonl_path, "w");
    if (fp) fclose(fp);
  }

  printf("\n=== Running %d matches for [%s] ===\n", NUM_BENCHMARK_MATCHES, label);

  for (int m = 0; m < NUM_BENCHMARK_MATCHES; m++) {
    uint64_t seed = 4000000ULL + (uint64_t)m * 31ULL;

    sim_match_config_t cfg = {
      .seed = seed,
      .max_duration_sec = 120.0f,
      .num_rivals = 8,
      .arena_radius = 3200.0f,
      .jsonl_log_path = jsonl_path
    };

    sim_world_t world;
    sim_init(&world, &cfg, weights);
    sim_match_result_t res = sim_run_match(&world);

    out_stats->survival_times[m] = res.survival_time;
    out_stats->max_masses[m] = res.max_mass;
    out_stats->kills[m] = res.kills;
    out_stats->death_causes[m] = res.death_cause;

    if ((m + 1) % 50 == 0) {
      printf("  Progress: %3d / %d matches completed...\n", m + 1, NUM_BENCHMARK_MATCHES);
    }
  }

  compute_stats(out_stats, NUM_BENCHMARK_MATCHES);
}

int main(int argc, char** argv) {
  (void)argc; (void)argv;
  printf("=========================================================================\n");
  printf("   YSLITHER PHASE 4 REPRODUCIBLE BENCHMARK: OLD BOT vs NEW BOT\n");
  printf("   Matches per bot: %d | Identical Seeds | Full Kinematics & Collisions\n", NUM_BENCHMARK_MATCHES);
  printf("=========================================================================\n");

  // 1. Old Bot Weights (Baseline conservative / weak clearance weights)
  sbot_weights_t old_weights;
  sbot_weights_init_default(&old_weights);
  old_weights.safety_margin_base = 10.0f; // Viejo margen bajo que causaba choques
  old_weights.weight_clearance = 1.0f;   // Sin ponderación de espacio abierto
  old_weights.weight_food = 6.0f;        // Codicia ciega de comida
  old_weights.weight_turn_penalty = 0.1f;// Giros erráticos
  old_weights.escape_enter_dist = 80.0f; // Reacción tardía de escape

  // 2. New Top Bot Weights (Phase 3 & Phase 4 Kinematic Rollouts & Hysteresis)
  sbot_weights_t new_weights;
  sbot_weights_init_default(&new_weights);

  bench_stats_t stats_old, stats_new;

  run_eval_batch("Old Bot (Baseline)", &old_weights, "telemetry_old_bot.jsonl", &stats_old);
  run_eval_batch("New Bot (Phase 3/4 Top)", &new_weights, "telemetry_new_bot.jsonl", &stats_new);

  // Print Comparison Summary Table
  printf("\n=========================================================================\n");
  printf("                    REPRODUCIBLE BENCHMARK RESULTS                       \n");
  printf("=========================================================================\n");
  printf("%-28s | %-16s | %-16s | %-12s\n", "Métrica", "Old Bot", "New Bot (Top)", "Mejora");
  printf("-----------------------------+------------------+------------------+-------------\n");

  float time_diff = ((stats_new.mean_survival - stats_old.mean_survival) / stats_old.mean_survival) * 100.0f;
  float med_time_diff = ((stats_new.median_survival - stats_old.median_survival) / stats_old.median_survival) * 100.0f;
  float mass_diff = ((stats_new.mean_max_mass - stats_old.mean_max_mass) / stats_old.mean_max_mass) * 100.0f;
  float kills_diff = stats_old.total_kills > 0 ?
    ((stats_new.total_kills - stats_old.total_kills) / (float)stats_old.total_kills) * 100.0f : 100.0f;

  printf("%-28s | %13.2f s   | %13.2f s   | %+9.1f %%\n", "Supervivencia Media (s)",
         stats_old.mean_survival, stats_new.mean_survival, time_diff);
  printf("%-28s | %13.2f s   | %13.2f s   | %+9.1f %%\n", "Supervivencia Mediana (s)",
         stats_old.median_survival, stats_new.median_survival, med_time_diff);
  printf("%-28s | %13.2f s   | %13.2f s   |      -      \n", "Desviación Estándar (s)",
         stats_old.std_survival, stats_new.std_survival);
  printf("%-28s | %13.1f     | %13.1f     | %+9.1f %%\n", "Masa Máxima Media",
         stats_old.mean_max_mass, stats_new.mean_max_mass, mass_diff);
  printf("%-28s | %13.1f     | %13.1f     |      -      \n", "Masa Máxima Mediana",
         stats_old.median_max_mass, stats_new.median_max_mass);
  printf("%-28s | %13d     | %13d     | %+9.1f %%\n", "Kills Totales",
         (int)stats_old.total_kills, (int)stats_new.total_kills, kills_diff);
  printf("-----------------------------+------------------+------------------+-------------\n");
  printf("Causas de Muerte (Old vs New):\n");
  printf("  - Choque Cabeza vs Cuerpo : %d (%.1f%%) vs %d (%.1f%%)\n",
         stats_old.deaths_body, (float)stats_old.deaths_body * 100.0f / (float)NUM_BENCHMARK_MATCHES,
         stats_new.deaths_body, (float)stats_new.deaths_body * 100.0f / (float)NUM_BENCHMARK_MATCHES);
  printf("  - Choque Cabeza vs Cabeza : %d (%.1f%%) vs %d (%.1f%%)\n",
         stats_old.deaths_head, (float)stats_old.deaths_head * 100.0f / (float)NUM_BENCHMARK_MATCHES,
         stats_new.deaths_head, (float)stats_new.deaths_head * 100.0f / (float)NUM_BENCHMARK_MATCHES);
  printf("  - Choque contra Borde     : %d (%.1f%%) vs %d (%.1f%%)\n",
         stats_old.deaths_border, (float)stats_old.deaths_border * 100.0f / (float)NUM_BENCHMARK_MATCHES,
         stats_new.deaths_border, (float)stats_new.deaths_border * 100.0f / (float)NUM_BENCHMARK_MATCHES);
  printf("  - Sobrevivió Límite Tiempo: %d (%.1f%%) vs %d (%.1f%%)\n",
         stats_old.survived_full, (float)stats_old.survived_full * 100.0f / (float)NUM_BENCHMARK_MATCHES,
         stats_new.survived_full, (float)stats_new.survived_full * 100.0f / (float)NUM_BENCHMARK_MATCHES);
  printf("=========================================================================\n");

  return 0;
}
