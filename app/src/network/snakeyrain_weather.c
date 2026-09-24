#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#include "snakeyrain_weather.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "../external/mongoose.h"

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "snakeyrain", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "snakeyrain", __VA_ARGS__)
#else
#define LOGI(...) printf("[SnakeyRain] " __VA_ARGS__); printf("\n")
#define LOGE(...) fprintf(stderr, "[SnakeyRain ERROR] " __VA_ARGS__); fprintf(stderr, "\n")
#endif

// Verified initial botstorm servers monitored by SnakeyRain Weather (https://snakeyrain.com/weather/)
static const rain_server_entry s_default_rain_servers[] = {
    {9637, "9637", "57.129.37.42",  444, "57.129.37.42:444",  "Frankfurt am Main",   "Europa",       "rain",     87,   577,  35,  true},
    {6698, "6698", "51.161.209.120", 444, "51.161.209.120:444", "Sydney",              "Oceanía",      "follow",   265,  530,  220, true},
    {2407, "2407", "57.129.37.44",   444, "57.129.37.44:444",   "Frankfurt am Main 2", "Europa",       "follow",   270,  546,  38,  true},
    {0,    "DNE",  "51.91.19.175",   444, "51.91.19.175:444",   "Roubaix",             "Europa",       "follow",   1032, 1076, 45,  true},
    {0,    "DNE",  "206.221.176.180",444, "206.221.176.180:444", "Piscataway",          "Norteamérica", "follow",   751,  1500, 110, true},
    {8127, "8127", "181.41.140.178", 444, "181.41.140.178:444", "Chiba",               "Asia",         "follow",   179,  384,  180, true},
    {7460, "7460", "15.235.218.24",  444, "15.235.218.24:444",  "Singapur",            "Asia",         "unfollow", 131,  476,  210, true},
    {2484, "2484", "45.158.39.122",  444, "45.158.39.122:444",  "São Paulo",           "Sudamérica",   "follow",   110,  343,  140, true},
    {5866, "5866", "162.19.235.91",  444, "162.19.235.91:444",  "Frankfurt am Main 3", "Europa",       "follow",   123,  256,  40,  true},
    {2819, "2819", "45.158.39.114",  444, "45.158.39.114:444",  "São Paulo 2",         "Sudamérica",   "follow",   75,   256,  145, true},
};

static const int s_default_count = sizeof(s_default_rain_servers) / sizeof(s_default_rain_servers[0]);

static rain_server_entry s_rain_servers[MAX_RAIN_SERVERS];
static int s_rain_server_count = 0;
static bool s_connected = false;
static bool s_initialized = false;
static bool s_running = false;
static bool s_is_rain_pinging = false;

static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t s_thread;
static bool s_force_refresh = false;

static int ping_one(const char* ip, int port) {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) return 999;

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 300000;  // 300 ms timeout
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
    close(sock);
    return 999;
  }

  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  int res = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
  clock_gettime(CLOCK_MONOTONIC, &t1);
  close(sock);

  if (res == 0) {
    double ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) * 1e-6;
    return (int)(ms < 1.0 ? 1 : ms);
  }
  return 999;
}

static void* rain_ping_thread_worker(void* arg) {
  (void)arg;
  int count;
  char ips[MAX_RAIN_SERVERS][32];
  int ports[MAX_RAIN_SERVERS];

  pthread_mutex_lock(&s_mutex);
  count = s_rain_server_count;
  for (int i = 0; i < count; i++) {
    strncpy(ips[i], s_rain_servers[i].ip, sizeof(ips[i]) - 1);
    ips[i][sizeof(ips[i]) - 1] = '\0';
    ports[i] = s_rain_servers[i].port;
  }
  pthread_mutex_unlock(&s_mutex);

  for (int i = 0; i < count && s_running; i++) {
    int p = ping_one(ips[i], ports[i]);
    pthread_mutex_lock(&s_mutex);
    if (i < s_rain_server_count) {
      s_rain_servers[i].ping_ms = p;
    }
    pthread_mutex_unlock(&s_mutex);
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 20000000L};
    nanosleep(&ts, NULL);  // 20ms polite delay
  }

  pthread_mutex_lock(&s_mutex);
  s_is_rain_pinging = false;
  pthread_mutex_unlock(&s_mutex);
  return NULL;
}

static void update_pings(void) {
  pthread_mutex_lock(&s_mutex);
  if (s_is_rain_pinging || !s_running) {
    pthread_mutex_unlock(&s_mutex);
    return;
  }
  s_is_rain_pinging = true;
  pthread_mutex_unlock(&s_mutex);

  pthread_t tid;
  if (pthread_create(&tid, NULL, rain_ping_thread_worker, NULL) == 0) {
    pthread_detach(tid);
  } else {
    pthread_mutex_lock(&s_mutex);
    s_is_rain_pinging = false;
    pthread_mutex_unlock(&s_mutex);
  }
}

static void parse_weather_json(const char* json_data, size_t json_len) {
  if (!json_data || json_len == 0) return;

  struct mg_str s = mg_str_n(json_data, json_len);
  rain_server_entry parsed[MAX_RAIN_SERVERS];
  int parsed_count = 0;

  for (int i = 0; i < MAX_RAIN_SERVERS; i++) {
    char path[64];
    snprintf(path, sizeof(path), "$[%d].server", i);
    char* srv_str = mg_json_get_str(s, path);
    if (!srv_str) break;

    rain_server_entry* r = &parsed[parsed_count];
    memset(r, 0, sizeof(*r));
    r->active = true;
    r->ping_ms = -1;

    // Parse server IP and Port (e.g. "57.129.37.42:443" -> IP="57.129.37.42", Port=444)
    char* colon = strchr(srv_str, ':');
    if (colon) {
      *colon = '\0';
      strncpy(r->ip, srv_str, sizeof(r->ip) - 1);
      int raw_port = atoi(colon + 1);
      // For native ws, use 444 if raw_port is standard 443 WSS port
      r->port = (raw_port == 443 || raw_port <= 0) ? 444 : raw_port;
    } else {
      strncpy(r->ip, srv_str, sizeof(r->ip) - 1);
      r->port = 444;
    }
    snprintf(r->full_addr, sizeof(r->full_addr), "%s:%d", r->ip, r->port);
    free(srv_str);

    // sID (number or string e.g. "DNE")
    snprintf(path, sizeof(path), "$[%d].sID", i);
    long sid_num = mg_json_get_long(s, path, -1);
    if (sid_num >= 0) {
      r->sid = (int)sid_num;
      snprintf(r->sid_str, sizeof(r->sid_str), "%d", r->sid);
    } else {
      char* sid_s = mg_json_get_str(s, path);
      if (sid_s) {
        strncpy(r->sid_str, sid_s, sizeof(r->sid_str) - 1);
        r->sid = atoi(sid_s);
        free(sid_s);
      } else {
        strncpy(r->sid_str, "----", sizeof(r->sid_str) - 1);
        r->sid = 0;
      }
    }

    // Bots alive count (bAli)
    snprintf(path, sizeof(path), "$[%d].bAli", i);
    r->bots_alive = (int)mg_json_get_long(s, path, 0);

    // Player count (pCt)
    snprintf(path, sizeof(path), "$[%d].pCt", i);
    r->player_count = (int)mg_json_get_long(s, path, 0);

    // Direction state (dirState)
    snprintf(path, sizeof(path), "$[%d].dirState", i);
    char* dir = mg_json_get_str(s, path);
    if (dir) {
      strncpy(r->dir_state, dir, sizeof(r->dir_state) - 1);
      free(dir);
    } else {
      strncpy(r->dir_state, "rain", sizeof(r->dir_state) - 1);
    }

    // City
    snprintf(path, sizeof(path), "$[%d].city", i);
    char* city = mg_json_get_str(s, path);
    if (city) {
      strncpy(r->city, city, sizeof(r->city) - 1);
      free(city);
    } else {
      strncpy(r->city, "Desconocido", sizeof(r->city) - 1);
    }

    // Continent
    snprintf(path, sizeof(path), "$[%d].cont", i);
    char* cont = mg_json_get_str(s, path);
    if (cont) {
      strncpy(r->cont, cont, sizeof(r->cont) - 1);
      free(cont);
    } else {
      strncpy(r->cont, "Mundial", sizeof(r->cont) - 1);
    }

    parsed_count++;
  }

  if (parsed_count > 0) {
    pthread_mutex_lock(&s_mutex);
    // Preserve existing pings if the same IP is present
    for (int i = 0; i < parsed_count; i++) {
      for (int j = 0; j < s_rain_server_count; j++) {
        if (strcmp(parsed[i].ip, s_rain_servers[j].ip) == 0 && s_rain_servers[j].ping_ms > 0) {
          parsed[i].ping_ms = s_rain_servers[j].ping_ms;
          break;
        }
      }
    }
    memcpy(s_rain_servers, parsed, sizeof(rain_server_entry) * parsed_count);
    s_rain_server_count = parsed_count;
    s_connected = true;
    pthread_mutex_unlock(&s_mutex);

    LOGI("Parsed %d live storm servers from SnakeyRain Weather Radar", parsed_count);
    update_pings();
  }
}

static void on_ws_event(struct mg_connection* c, int ev, void* ev_data) {
  (void)c;
  if (ev == MG_EV_WS_OPEN) {
    LOGI("Connected to SnakeyRain Weather Radar WebSocket (wss://snakeyrain.com:8444/)");
    pthread_mutex_lock(&s_mutex);
    s_connected = true;
    pthread_mutex_unlock(&s_mutex);
  } else if (ev == MG_EV_WS_MSG) {
    struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
    parse_weather_json(wm->data.buf, wm->data.len);
  } else if (ev == MG_EV_CLOSE || ev == MG_EV_ERROR) {
    pthread_mutex_lock(&s_mutex);
    s_connected = false;
    pthread_mutex_unlock(&s_mutex);
  }
}

static void* weather_worker_thread(void* arg) {
  (void)arg;
  LOGI("SnakeyRain Weather Radar worker thread started.");

  while (s_running) {
    struct mg_mgr mgr;
    mg_log_set(MG_LL_NONE);
    mg_mgr_init(&mgr);

    LOGI("Connecting to wss://snakeyrain.com:8444/ ...");
    struct mg_connection* c = mg_ws_connect(&mgr, "wss://snakeyrain.com:8444/", on_ws_event, NULL,
                                            "Origin: https://snakeyrain.com\r\n");
    if (c) {
      struct mg_tls_opts opts = {.name = mg_str("snakeyrain.com"), .skip_verification = 1};
      mg_tls_init(c, &opts);

      time_t start_time = time(NULL);
      while (s_running && !s_force_refresh) {
        mg_mgr_poll(&mgr, 100);

        // Periodic ping refresh every 30 seconds
        if (time(NULL) - start_time >= 30) {
          start_time = time(NULL);
          update_pings();
        }

        pthread_mutex_lock(&s_mutex);
        bool connected = s_connected;
        pthread_mutex_unlock(&s_mutex);
        if (!connected && time(NULL) - start_time > 10) {
          break;
        }
      }
    }

    mg_mgr_free(&mgr);

    pthread_mutex_lock(&s_mutex);
    s_force_refresh = false;
    s_connected = false;
    pthread_mutex_unlock(&s_mutex);

    // Polite pause before reconnecting unless forced refresh
    for (int wait_sec = 0; wait_sec < 5 && s_running && !s_force_refresh; wait_sec++) {
      sleep(1);
    }
  }

  LOGI("SnakeyRain Weather Radar worker thread terminated.");
  return NULL;
}

void snakeyrain_weather_init(void) {
  pthread_mutex_lock(&s_mutex);
  if (s_initialized) {
    pthread_mutex_unlock(&s_mutex);
    return;
  }
  s_initialized = true;
  s_running = true;

  // Initialize with seed storm servers
  memcpy(s_rain_servers, s_default_rain_servers, sizeof(s_default_rain_servers));
  s_rain_server_count = s_default_count;
  pthread_mutex_unlock(&s_mutex);

  pthread_create(&s_thread, NULL, weather_worker_thread, NULL);
  update_pings();
}

void snakeyrain_weather_destroy(void) {
  pthread_mutex_lock(&s_mutex);
  if (!s_initialized) {
    pthread_mutex_unlock(&s_mutex);
    return;
  }
  s_running = false;
  s_force_refresh = true;
  pthread_mutex_unlock(&s_mutex);

  pthread_join(s_thread, NULL);
  s_initialized = false;
}

void snakeyrain_weather_refresh(void) {
  pthread_mutex_lock(&s_mutex);
  s_force_refresh = true;
  pthread_mutex_unlock(&s_mutex);
  update_pings();
}

int snakeyrain_weather_count(void) {
  pthread_mutex_lock(&s_mutex);
  int c = s_rain_server_count;
  pthread_mutex_unlock(&s_mutex);
  return c;
}

bool snakeyrain_weather_get_copy(int index, rain_server_entry* out_entry) {
  if (!out_entry) return false;
  pthread_mutex_lock(&s_mutex);
  if (index < 0 || index >= s_rain_server_count) {
    pthread_mutex_unlock(&s_mutex);
    return false;
  }
  *out_entry = s_rain_servers[index];
  pthread_mutex_unlock(&s_mutex);
  return true;
}

bool snakeyrain_weather_is_connected(void) {
  pthread_mutex_lock(&s_mutex);
  bool c = s_connected;
  pthread_mutex_unlock(&s_mutex);
  return c;
}

int snakeyrain_weather_total_bots(void) {
  pthread_mutex_lock(&s_mutex);
  int sum = 0;
  for (int i = 0; i < s_rain_server_count; i++) {
    sum += s_rain_servers[i].bots_alive;
  }
  pthread_mutex_unlock(&s_mutex);
  return sum;
}

int snakeyrain_weather_get_ping_by_ip(const char* target_ip) {
  if (!target_ip || target_ip[0] == '\0') return -1;
  pthread_mutex_lock(&s_mutex);
  for (int i = 0; i < s_rain_server_count; i++) {
    if (strcmp(s_rain_servers[i].full_addr, target_ip) == 0 ||
        strcmp(s_rain_servers[i].ip, target_ip) == 0) {
      int p = s_rain_servers[i].ping_ms;
      pthread_mutex_unlock(&s_mutex);
      return p;
    }
  }
  pthread_mutex_unlock(&s_mutex);
  return -1;
}

const char* snakeyrain_weather_get_name_by_ip(const char* target_ip, int* out_bots, char* out_state, int state_len) {
  if (!target_ip || target_ip[0] == '\0') return NULL;
  static char name_buf[128];

  pthread_mutex_lock(&s_mutex);
  for (int i = 0; i < s_rain_server_count; i++) {
    if (strcmp(s_rain_servers[i].full_addr, target_ip) == 0 ||
        strcmp(s_rain_servers[i].ip, target_ip) == 0) {
      if (out_bots) *out_bots = s_rain_servers[i].bots_alive;
      if (out_state && state_len > 0) {
        strncpy(out_state, s_rain_servers[i].dir_state, state_len - 1);
        out_state[state_len - 1] = '\0';
      }
      snprintf(name_buf, sizeof(name_buf), "Lluvia: %s (#%s - %d bots)",
               s_rain_servers[i].city, s_rain_servers[i].sid_str, s_rain_servers[i].bots_alive);
      pthread_mutex_unlock(&s_mutex);
      return name_buf;
    }
  }
  pthread_mutex_unlock(&s_mutex);
  return NULL;
}
