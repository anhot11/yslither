#ifndef SERVER_LIST_H
#define SERVER_LIST_H

#include <stdbool.h>
#include "snakeyrain_weather.h"

#define MAX_SERVERS 64

typedef struct {
  int sid;
  char sid_str[16];
  char ip[32];
  int port;
  char full_addr[64];
  char name[64];
  char region[16]; // "US-W", "US-C", "US-E", "EU", "SA", "AS", "ME", "AF", or "Lluvia"
  int ping_ms;     // -1 = measuring, >0 = ms, 999 = timeout
  int players;
  bool active;

  // SnakeyRain Weather / Botstorm attributes
  int bots_alive;
  char storm_state[24]; // "rain", "follow", "unfollow"
  char city[48];
  char cont[32];
} server_entry;

void server_list_init(void);
int server_list_count(void);
int server_list_official_count(void);
server_entry* server_list_get(int index);
bool server_list_get_copy(int index, server_entry* out_entry);
void server_list_refresh_pings(void);
const char* server_list_get_best_ip(void);
const char* server_list_get_best_ip_by_region(const char* region);
const char* server_list_get_fallback_ip(int attempt);
const char* server_list_get_name_by_ip(const char* target_ip);
int server_list_get_ping_by_ip(const char* target_ip);
bool server_list_is_pinging(void);

#endif
