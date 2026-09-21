#ifndef CALLBACK_H
#define CALLBACK_H

#include "../external/mongoose.h"

void server_callback(struct mg_connection* c, int ev, void *ev_data);
void decode_secret(const uint8_t* packet, size_t packet_len, uint8_t* result);

#endif