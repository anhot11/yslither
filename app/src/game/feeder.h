#ifndef FEEDER_H
#define FEEDER_H

#include <stdbool.h>

struct tenv;
typedef struct tenv tenv;

#define MAX_FEEDER_BOTS 5

void feeder_init(tenv* env);
void feeder_update(tenv* env);
void feeder_destroy(tenv* env);

int feeder_get_active_count(void);
int feeder_get_target_count(void);
void feeder_set_target_count(int count);
bool feeder_is_enabled(void);
void feeder_set_enabled(bool enabled);
void feeder_toggle_enabled(void);
float feeder_get_closest_dist(void);

#endif
