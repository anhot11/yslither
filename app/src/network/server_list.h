#ifndef SERVER_LIST_H
#define SERVER_LIST_H

#include <stdbool.h>

#define MAX_SERVERS 64

typedef struct {
  int sid;
  char ip[32];
  int port;
  char name[48];
  char region[16];
  int ping_ms;     // -1 = measuring, >0 = ms, 999 = timeout
  int players;
  bool active;
} server_entry;

void server_list_init(void);
int server_list_count(void);
server_entry* server_list_get(int index);
void server_list_refresh_pings(void);
const char* server_list_get_best_ip(void);
const char* server_list_get_name_by_ip(const char* target_ip);
int server_list_get_ping_by_ip(const char* target_ip);
bool server_list_is_pinging(void);

#endif
