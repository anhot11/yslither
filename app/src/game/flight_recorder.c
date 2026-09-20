#include "flight_recorder.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "../user.h"
#include "sbot.h"

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "flight_recorder", __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__)
#endif

static telemetry_frame_t s_ring[TELEMETRY_RING_SIZE];
static int s_head = 0;
static int s_count = 0;
static bool s_death_recorded = false;
static char s_last_death_summary[256] = "Ninguna muerte registrada aun.";

void flight_recorder_init(void) {
  memset(s_ring, 0, sizeof(s_ring));
  s_head = 0;
  s_count = 0;
  s_death_recorded = false;
}

void flight_recorder_record_frame(tenv* env) {
  tuser_data* usr = env->usr;
  user_settings* usrs = &usr->usrs;
  game_data* gdata = &usr->gdata;

  // Skip recording if Debug/Logs are disabled
  if (!usrs->debug_logs_enabled) return;

  int ns = tdarray_length(gdata->data.snakes);
  if (ns == 0) return;

  snake* me = NULL;
  for (int i = 0; i < ns; i++) {
    if (gdata->data.snakes[i].id == gdata->data.snake_id) {
      me = gdata->data.snakes + i;
      break;
    }
  }
  if (!me) me = gdata->data.snakes + (ns - 1);
  if (me->dead) return;

  telemetry_frame_t* tf = &s_ring[s_head];
  tf->time_sec = (float)glfwGetTime();
  tf->my_x = me->xx;
  tf->my_y = me->yy;
  tf->my_ang = me->ang;
  tf->my_speed = me->sp;
  tf->score = gdata->data.score;
  tf->kills = gdata->data.kills;
  tf->accel = gdata->bot.output.accel;
  tf->goal_x = gdata->bot.output.xm;
  tf->goal_y = gdata->bot.output.ym;

  // Search nearest threat among other snakes
  float min_d2 = 9999999.0f;
  float threat_x = 0.0f, threat_y = 0.0f;
  int threats = 0;

  for (int i = 0; i < ns; i++) {
    snake* s = gdata->data.snakes + i;
    if (s->id == me->id || s->dead) continue;
    float dx = s->xx - me->xx;
    float dy = s->yy - me->yy;
    float d2 = dx * dx + dy * dy;
    if (d2 < 600.0f * 600.0f) {
      threats++;
      if (d2 < min_d2) {
        min_d2 = d2;
        threat_x = s->xx;
        threat_y = s->yy;
      }
    }
  }

  tf->threat_count = threats;
  tf->nearest_threat_dist = sqrtf(min_d2);
  tf->nearest_threat_x = threat_x;
  tf->nearest_threat_y = threat_y;

  s_head = (s_head + 1) % TELEMETRY_RING_SIZE;
  if (s_count < TELEMETRY_RING_SIZE) s_count++;

  // Reset death recorded flag if alive
  s_death_recorded = false;
}

void flight_recorder_on_death(tenv* env) {
  tuser_data* usr = env->usr;
  user_settings* usrs = &usr->usrs;
  game_data* gdata = &usr->gdata;

  // Skip if logs/debug are disabled or already dumped for this death
  if (!usrs->debug_logs_enabled) return;
  if (s_death_recorded) return;
  s_death_recorded = true;

  if (s_count == 0) return;

  // Retrieve last recorded frame before death
  int last_idx = (s_head - 1 + TELEMETRY_RING_SIZE) % TELEMETRY_RING_SIZE;
  telemetry_frame_t* last_tf = &s_ring[last_idx];

  // Diagnose death cause
  const char* cause = "COLISION CON SERPIENTE ENEMIGA";
  float c_dx = last_tf->my_x - gdata->data.grd;
  float c_dy = last_tf->my_y - gdata->data.grd;
  float dist_to_center = sqrtf(c_dx * c_dx + c_dy * c_dy);
  if (dist_to_center >= (gdata->data.flux_grd - 60.0f)) {
    cause = "COLISION CON BORDE DEL MAPA";
  } else if (last_tf->nearest_threat_dist < 70.0f) {
    cause = "CORTE FRONTAL / INTERCEPCION RAPIDA";
  } else if (last_tf->threat_count >= 3) {
    cause = "ENCERRAMIENTO MULTIPLE (TRAMPA DE SERPIENTES)";
  }

  snprintf(s_last_death_summary, sizeof(s_last_death_summary),
           "Puntos: %d | Kills: %d | Causa: %s (Amenaza: %.0f px)",
           last_tf->score, last_tf->kills, cause, last_tf->nearest_threat_dist);

  LOGI("FLIGHT RECORDER DIAGNOSIS: %s", s_last_death_summary);

  // Write telemetry dump JSON file to local app files
  FILE* fp = fopen("telemetry_death_latest.json", "w");
  if (fp) {
    fprintf(fp, "{\n");
    fprintf(fp, "  \"timestamp\": %ld,\n", (long)time(NULL));
    fprintf(fp, "  \"version\": \"%s\",\n", APP_VERSION);
    fprintf(fp, "  \"bot_mode\": %d,\n", usrs->bot_mode);
    fprintf(fp, "  \"score\": %d,\n", last_tf->score);
    fprintf(fp, "  \"kills\": %d,\n", last_tf->kills);
    fprintf(fp, "  \"cause\": \"%s\",\n", cause);
    fprintf(fp, "  \"final_pos\": {\"x\": %.2f, \"y\": %.2f},\n", last_tf->my_x, last_tf->my_y);
    fprintf(fp, "  \"threat_dist\": %.2f,\n", last_tf->nearest_threat_dist);
    fprintf(fp, "  \"recent_frames\": [\n");

    int frames_to_dump = (s_count < 60) ? s_count : 60;
    int start_idx = (s_head - frames_to_dump + TELEMETRY_RING_SIZE) % TELEMETRY_RING_SIZE;
    for (int i = 0; i < frames_to_dump; i++) {
      int idx = (start_idx + i) % TELEMETRY_RING_SIZE;
      telemetry_frame_t* f = &s_ring[idx];
      fprintf(fp, "    {\"t\": %.2f, \"x\": %.1f, \"y\": %.1f, \"ang\": %.2f, \"sp\": %.2f, \"thr\": %d, \"d\": %.1f}%s\n",
              f->time_sec, f->my_x, f->my_y, f->my_ang, f->my_speed, f->threat_count, f->nearest_threat_dist,
              (i == frames_to_dump - 1) ? "" : ",");
    }
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");
    fclose(fp);
    LOGI("Flight Recorder: Dump successfully saved to telemetry_death_latest.json");
  }
}

bool flight_recorder_has_death_event(void) {
  return (strcmp(s_last_death_summary, "Ninguna muerte registrada aun.") != 0);
}

const char* flight_recorder_get_last_death_summary(void) {
  return s_last_death_summary;
}
