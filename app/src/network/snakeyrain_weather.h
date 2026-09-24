#ifndef SNAKEYRAIN_WEATHER_H
#define SNAKEYRAIN_WEATHER_H

#include <stdbool.h>

#define MAX_RAIN_SERVERS 32

typedef struct {
  int sid;
  char sid_str[16];     // e.g. "9637", "6698", or "DNE"
  char ip[32];          // e.g. "57.129.37.42"
  int port;             // 444
  char full_addr[64];   // e.g. "57.129.37.42:444"
  char city[48];        // e.g. "Frankfurt am Main"
  char cont[32];        // e.g. "Europe"
  char dir_state[24];   // "rain", "follow", "unfollow"
  int bots_alive;       // bAli
  int player_count;     // pCt
  int ping_ms;          // -1 = measuring, >0 = ms, 999 = timeout
  bool active;
} rain_server_entry;

void snakeyrain_weather_init(void);
void snakeyrain_weather_destroy(void);
void snakeyrain_weather_refresh(void);
int snakeyrain_weather_count(void);
bool snakeyrain_weather_get_copy(int index, rain_server_entry* out_entry);
bool snakeyrain_weather_is_connected(void);
int snakeyrain_weather_total_bots(void);
int snakeyrain_weather_get_ping_by_ip(const char* target_ip);
const char* snakeyrain_weather_get_name_by_ip(const char* target_ip, int* out_bots, char* out_state, int state_len);

#endif
