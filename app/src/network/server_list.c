#define _POSIX_C_SOURCE 199309L
#include "server_list.h"

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

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "server_list", __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__); printf("\n")
#endif

static server_entry s_servers[] = {
    // North America (US & Canada) - pre-sorted by performance
    {8400, "8400", "23.29.125.178",  444, "23.29.125.178:444",  "US West 2 (Silicon V.)", "US-W", 20, 20, true, 0, "", "Silicon Valley", "Norteamérica"},
    {7870, "7870", "15.204.212.200", 444, "15.204.212.200:444", "US Central (Chicago)",  "US-C", 35, 35, true, 0, "", "Chicago", "Norteamérica"},
    {5724, "5724", "192.211.52.146", 444, "192.211.52.146:444", "US West (Los Ángeles)", "US-W", 40, 40, true, 0, "", "Los Ángeles", "Norteamérica"},
    {7531, "7531", "51.161.209.120", 444, "51.161.209.120:444", "North America (Canadá)","NA",   40, 40, true, 0, "", "Canadá", "Norteamérica"},
    {4164, "4164", "23.227.195.74",  444, "23.227.195.74:444",  "US East (Atlanta)",     "US-E", 45, 45, true, 0, "", "Atlanta", "Norteamérica"},
    {4371, "4371", "66.165.238.34",  444, "66.165.238.34:444",  "US Central 3 (Texas)",   "US-C", 50, 50, true, 0, "", "Texas", "Norteamérica"},
    {5120, "5120", "15.204.213.229", 444, "15.204.213.229:444", "US Central 2 (Dallas)", "US-C", 60, 60, true, 0, "", "Dallas", "Norteamérica"},
    {2260, "2260", "107.155.103.54", 444, "107.155.103.54:444", "US East 3 (Virginia)",  "US-E", 65, 65, true, 0, "", "Virginia", "Norteamérica"},
    {3619, "3619", "107.155.98.194", 444, "107.155.98.194:444", "US South (Miami)",      "US-S", 70, 70, true, 0, "", "Miami", "Norteamérica"},
    {7771, "7771", "107.155.76.138", 444, "107.155.76.138:444", "US East 2 (New York)",  "US-E", 105, 105, true, 0, "", "New York", "Norteamérica"},
    {8828, "8828", "148.113.20.151", 444, "148.113.20.151:444", "North America 2",       "NA",   110, 30, true, 0, "", "North America", "Norteamérica"},

    // Europe
    {6622, "6622", "198.244.231.35", 444, "198.244.231.35:444", "Europa (Londres)",      "EU",   180, 240, true, 0, "", "Londres", "Europa"},
    {3586, "3586", "185.199.38.101", 444, "185.199.38.101:444", "Europa (Varsovia)",     "EU",   180, 180, true, 0, "", "Varsovia", "Europa"},
    {9670, "9670", "217.138.162.194",444, "217.138.162.194:444","Europa 2 (UK)",         "EU",   190, 190, true, 0, "", "UK", "Europa"},
    {5574, "5574", "57.128.202.109", 444, "57.128.202.109:444", "Europa (Gravelines)",   "EU",   210, 210, true, 0, "", "Gravelines", "Europa"},
    {7806, "7806", "92.222.100.202", 444, "92.222.100.202:444", "Europa (París)",        "EU",   240, 240, true, 0, "", "París", "Europa"},
    {4369, "4369", "135.125.74.228", 444, "135.125.74.228:444", "Europa (Frankfurt)",    "EU",   250, 250, true, 0, "", "Frankfurt", "Europa"},

    // South America
    {8848, "8848", "181.41.140.146", 444, "181.41.140.146:444", "Chile (Santiago)",      "SA",   110, 110, true, 0, "", "Santiago", "Sudamérica"},
    {4571, "4571", "181.41.140.170", 444, "181.41.140.170:444", "Argentina (Buenos Aires)","SA", 115, 115, true, 0, "", "Buenos Aires", "Sudamérica"},
    {4263, "4263", "57.129.37.42",   444, "57.129.37.42:444",   "Brasil (Sao Paulo)",    "SA",   120, 120, true, 0, "", "Sao Paulo", "Sudamérica"},

    // Asia & Middle East
    {3310, "3310", "45.158.39.122",  444, "45.158.39.122:444",  "Asia 2 (Tokio)",        "AS",   110, 30, true, 0, "", "Tokio", "Asia"},
    {7979, "7979", "103.4.30.88",    444, "103.4.30.88:444",    "Asia (India / Mumbai)", "AS",   130, 80, true, 0, "", "Mumbai", "Asia"},
    {2220, "2220", "15.235.218.24",  444, "15.235.218.24:444",  "Asia (Singapur)",       "AS",   135, 95, true, 0, "", "Singapur", "Asia"},
    {2878, "2878", "94.72.180.82",   444, "94.72.180.82:444",   "Medio Oriente 2",       "ME",   135, 135, true, 0, "", "Medio Oriente", "Asia"},
    {4490, "4490", "94.20.222.150",  444, "94.20.222.150:444",  "Medio Oriente (Dubái)", "ME",   140, 140, true, 0, "", "Dubái", "Asia"},

    // Africa
    {7376, "7376", "102.218.213.17", 444, "102.218.213.17:444", "África (Johannesburgo)","AF",   150, 50, true, 0, "", "Johannesburgo", "África"},
};

static const int s_server_count = sizeof(s_servers) / sizeof(s_servers[0]);
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t s_ping_thread;
static bool s_is_pinging = false;
static bool s_initialized = false;

static int ping_one_server(const char* ip, int port) {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) return 999;

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 400000;  // 400 ms fast timeout
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

static int server_ping_cmp(const void* a, const void* b) {
  const server_entry* sa = (const server_entry*)a;
  const server_entry* sb = (const server_entry*)b;
  int pa = (sa->ping_ms > 0 && sa->ping_ms < 999) ? sa->ping_ms : 9999;
  int pb = (sb->ping_ms > 0 && sb->ping_ms < 999) ? sb->ping_ms : 9999;
  return pa - pb;
}

static void* ping_worker_thread(void* arg) {
  (void)arg;
  LOGI("Starting background server ping measurement...");

  for (int i = 0; i < s_server_count; i++) {
    char ip[32];
    int port;
    pthread_mutex_lock(&s_mutex);
    strcpy(ip, s_servers[i].ip);
    port = s_servers[i].port;
    pthread_mutex_unlock(&s_mutex);

    int ms = ping_one_server(ip, port);

    pthread_mutex_lock(&s_mutex);
    s_servers[i].ping_ms = ms;
    pthread_mutex_unlock(&s_mutex);

    struct timespec ts = {.tv_sec = 0, .tv_nsec = 50000000L};
    nanosleep(&ts, NULL); // 50ms pause between checks
  }

  pthread_mutex_lock(&s_mutex);
  qsort(s_servers, s_server_count, sizeof(server_entry), server_ping_cmp);
  s_is_pinging = false;
  pthread_mutex_unlock(&s_mutex);

  LOGI("Server ping measurement finished (sorted by lowest ping).");
  return NULL;
}

void server_list_init(void) {
  if (s_initialized) return;
  s_initialized = true;

  // Initialize SnakeyRain Weather Radar for Lluvia zone
  snakeyrain_weather_init();
}

int server_list_count(void) {
  return s_server_count + snakeyrain_weather_count();
}

int server_list_official_count(void) {
  return s_server_count;
}

server_entry* server_list_get(int index) {
  if (index < 0 || index >= s_server_count) return NULL;
  return &s_servers[index];
}

bool server_list_get_copy(int index, server_entry* out_entry) {
  if (index < 0 || !out_entry) return false;

  if (index < s_server_count) {
    pthread_mutex_lock(&s_mutex);
    *out_entry = s_servers[index];
    pthread_mutex_unlock(&s_mutex);
    return true;
  }

  // Lluvia Zone server from SnakeyRain Weather
  int rain_idx = index - s_server_count;
  rain_server_entry r;
  if (!snakeyrain_weather_get_copy(rain_idx, &r)) {
    return false;
  }

  memset(out_entry, 0, sizeof(*out_entry));
  out_entry->sid = r.sid;
  strncpy(out_entry->sid_str, r.sid_str, sizeof(out_entry->sid_str) - 1);
  strncpy(out_entry->ip, r.ip, sizeof(out_entry->ip) - 1);
  out_entry->port = r.port;
  strncpy(out_entry->full_addr, r.full_addr, sizeof(out_entry->full_addr) - 1);
  snprintf(out_entry->name, sizeof(out_entry->name), "🌧️ %s (#%s)", r.city, r.sid_str);
  strncpy(out_entry->region, "Lluvia", sizeof(out_entry->region) - 1);
  out_entry->ping_ms = r.ping_ms;
  out_entry->players = r.player_count;
  out_entry->bots_alive = r.bots_alive;
  strncpy(out_entry->storm_state, r.dir_state, sizeof(out_entry->storm_state) - 1);
  strncpy(out_entry->city, r.city, sizeof(out_entry->city) - 1);
  strncpy(out_entry->cont, r.cont, sizeof(out_entry->cont) - 1);
  out_entry->active = r.active;
  return true;
}

void server_list_refresh_pings(void) {
  // Refresh SnakeyRain Weather radar
  snakeyrain_weather_refresh();

  pthread_mutex_lock(&s_mutex);
  if (s_is_pinging) {
    pthread_mutex_unlock(&s_mutex);
    return;
  }
  s_is_pinging = true;
  for (int i = 0; i < s_server_count; i++) {
    s_servers[i].ping_ms = -1;  // measuring
  }
  pthread_mutex_unlock(&s_mutex);

  pthread_create(&s_ping_thread, NULL, ping_worker_thread, NULL);
  pthread_detach(s_ping_thread);
}

bool server_list_is_pinging(void) {
  pthread_mutex_lock(&s_mutex);
  bool ret = s_is_pinging;
  pthread_mutex_unlock(&s_mutex);
  return ret;
}

const char* server_list_get_best_ip(void) {
  pthread_mutex_lock(&s_mutex);
  int best_ping = 9999;
  int best_idx = -1;

  for (int i = 0; i < s_server_count; i++) {
    if (s_servers[i].ping_ms > 0 && s_servers[i].ping_ms < best_ping) {
      best_ping = s_servers[i].ping_ms;
      best_idx = i;
    }
  }

  static char best_addr[64];
  if (best_idx >= 0) {
    snprintf(best_addr, sizeof(best_addr), "%s:%d", s_servers[best_idx].ip, s_servers[best_idx].port);
  } else {
    // Verified fast default server (Silicon Valley)
    strncpy(best_addr, "23.29.125.178:444", sizeof(best_addr));
  }
  pthread_mutex_unlock(&s_mutex);
  return best_addr;
}

const char* server_list_get_best_ip_by_region(const char* region) {
  if (!region || strcmp(region, "Todas") == 0) {
    return server_list_get_best_ip();
  }

  // If looking for best Lluvia server
  if (strcmp(region, "Lluvia") == 0) {
    int rain_count = snakeyrain_weather_count();
    int best_ping = 9999;
    static char best_rain_addr[64] = "57.129.37.42:444";
    for (int i = 0; i < rain_count; i++) {
      rain_server_entry r;
      if (snakeyrain_weather_get_copy(i, &r) && r.ping_ms > 0 && r.ping_ms < best_ping) {
        best_ping = r.ping_ms;
        strncpy(best_rain_addr, r.full_addr, sizeof(best_rain_addr) - 1);
      }
    }
    return best_rain_addr;
  }

  pthread_mutex_lock(&s_mutex);
  int best_ping = 9999;
  int best_idx = -1;
  for (int i = 0; i < s_server_count; i++) {
    if (strstr(s_servers[i].region, region) != NULL || strcmp(s_servers[i].region, region) == 0) {
      if (s_servers[i].ping_ms > 0 && s_servers[i].ping_ms < best_ping) {
        best_ping = s_servers[i].ping_ms;
        best_idx = i;
      }
    }
  }

  static char regional_addr[64];
  if (best_idx >= 0) {
    snprintf(regional_addr, sizeof(regional_addr), "%s:%d", s_servers[best_idx].ip, s_servers[best_idx].port);
    pthread_mutex_unlock(&s_mutex);
    return regional_addr;
  }
  pthread_mutex_unlock(&s_mutex);
  return server_list_get_best_ip();
}

const char* server_list_get_fallback_ip(int attempt) {
  pthread_mutex_lock(&s_mutex);
  int valid_indices[MAX_SERVERS];
  int valid_count = 0;
  for (int i = 0; i < s_server_count; i++) {
    if (s_servers[i].ping_ms > 0 && s_servers[i].ping_ms < 999) {
      valid_indices[valid_count++] = i;
    }
  }

  static char fb_addr[64];
  if (valid_count > 0) {
    int sel = valid_indices[attempt % valid_count];
    snprintf(fb_addr, sizeof(fb_addr), "%s:%d", s_servers[sel].ip, s_servers[sel].port);
  } else {
    static const char* const k_resilient[] = {
      "23.29.125.178:444",
      "23.227.195.74:444",
      "15.204.213.229:444",
      "15.204.212.200:444",
      "192.211.52.146:444"
    };
    int n_res = sizeof(k_resilient) / sizeof(k_resilient[0]);
    strncpy(fb_addr, k_resilient[attempt % n_res], sizeof(fb_addr));
  }
  pthread_mutex_unlock(&s_mutex);
  return fb_addr;
}

const char* server_list_get_name_by_ip(const char* target_ip) {
  if (!target_ip || target_ip[0] == '\0') return "Servidor Desconocido";

  // Check SnakeyRain Lluvia Zone first to display live botstorm badges
  int bots = 0;
  char storm[24] = {0};
  const char* rain_name = snakeyrain_weather_get_name_by_ip(target_ip, &bots, storm, sizeof(storm));
  if (rain_name) {
    return rain_name;
  }

  // Check official regional servers
  pthread_mutex_lock(&s_mutex);
  for (int i = 0; i < s_server_count; i++) {
    if (strcmp(s_servers[i].full_addr, target_ip) == 0 ||
        strcmp(s_servers[i].ip, target_ip) == 0) {
      const char* name = s_servers[i].name;
      pthread_mutex_unlock(&s_mutex);
      return name;
    }
  }
  pthread_mutex_unlock(&s_mutex);

  return target_ip;
}

int server_list_get_ping_by_ip(const char* target_ip) {
  if (!target_ip || target_ip[0] == '\0') return -1;

  pthread_mutex_lock(&s_mutex);
  for (int i = 0; i < s_server_count; i++) {
    if (strcmp(s_servers[i].full_addr, target_ip) == 0 ||
        strcmp(s_servers[i].ip, target_ip) == 0) {
      int ping = s_servers[i].ping_ms;
      pthread_mutex_unlock(&s_mutex);
      return ping;
    }
  }
  pthread_mutex_unlock(&s_mutex);

  // Check SnakeyRain Weather Radar
  int rain_ping = snakeyrain_weather_get_ping_by_ip(target_ip);
  if (rain_ping >= 0) return rain_ping;

  return -1;
}
