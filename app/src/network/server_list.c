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
#define LOGI(...) printf(__VA_ARGS__)
#endif

static server_entry s_servers[] = {
    {"192.211.52.146", 444, "US West (Los Ángeles)", "US-W", -1, 40, true},
    {"23.227.195.74",  444, "US East (Atlanta)",     "US-E", -1, 45, true},
    {"15.204.212.200", 444, "US Central (Chicago)",  "US-C", -1, 35, true},
    {"15.204.213.229", 444, "US Central 2 (Dallas)", "US-C", -1, 60, true},
    {"107.155.98.194", 444, "US South (Miami)",      "US-S", -1, 70, true},
    {"23.29.125.178",  444, "US West 2 (Silicon V.)", "US-W", -1, 20, true},
    {"135.125.74.228", 444, "Europa (Frankfurt)",    "EU",   -1, 250, true},
    {"92.222.100.202", 444, "Europa (París)",        "EU",   -1, 240, true},
    {"198.244.231.35", 444, "Europa (Londres)",      "EU",   -1, 240, true},
    {"107.155.76.138", 444, "US East 2 (New York)",  "US-E", -1, 105, true},
    {"45.158.39.122",  444, "Asia (Singapur)",       "AS",   -1, 30, true},
    {"66.165.238.34",  444, "US Central 3 (Texas)",   "US-C", -1, 50, true},
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
  tv.tv_sec = 1;
  tv.tv_usec = 200000;  // 1.2 sec timeout
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
  }

  pthread_mutex_lock(&s_mutex);
  s_is_pinging = false;
  pthread_mutex_unlock(&s_mutex);

  LOGI("Server ping measurement finished.");
  return NULL;
}

void server_list_init(void) {
  if (s_initialized) return;
  s_initialized = true;
  server_list_refresh_pings();
}

int server_list_count(void) {
  return s_server_count;
}

server_entry* server_list_get(int index) {
  if (index < 0 || index >= s_server_count) return NULL;
  return &s_servers[index];
}

void server_list_refresh_pings(void) {
  pthread_mutex_lock(&s_mutex);
  if (s_is_pinging) {
    pthread_mutex_unlock(&s_mutex);
    return;
  }
  s_is_pinging = true;
  for (int i = 0; i < s_server_count; i++) {
    s_servers[i].ping_ms = -1;  // set to measuring
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
  int best_idx = 0;

  for (int i = 0; i < s_server_count; i++) {
    if (s_servers[i].ping_ms > 0 && s_servers[i].ping_ms < best_ping) {
      best_ping = s_servers[i].ping_ms;
      best_idx = i;
    }
  }

  static char best_addr[64];
  snprintf(best_addr, sizeof(best_addr), "%s:%d", s_servers[best_idx].ip, s_servers[best_idx].port);
  pthread_mutex_unlock(&s_mutex);
  return best_addr;
}

const char* server_list_get_name_by_ip(const char* target_ip) {
  if (!target_ip || target_ip[0] == '\0') return "Servidor Desconocido";

  pthread_mutex_lock(&s_mutex);
  for (int i = 0; i < s_server_count; i++) {
    char full[64];
    snprintf(full, sizeof(full), "%s:%d", s_servers[i].ip, s_servers[i].port);
    if (strcmp(full, target_ip) == 0 || strcmp(s_servers[i].ip, target_ip) == 0) {
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
    char full[64];
    snprintf(full, sizeof(full), "%s:%d", s_servers[i].ip, s_servers[i].port);
    if (strcmp(full, target_ip) == 0 || strcmp(s_servers[i].ip, target_ip) == 0) {
      int ping = s_servers[i].ping_ms;
      pthread_mutex_unlock(&s_mutex);
      return ping;
    }
  }
  pthread_mutex_unlock(&s_mutex);
  return -1;
}
