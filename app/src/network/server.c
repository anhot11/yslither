#include "server.h"

#include "../user.h"
#include "callback.h"

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "yslither_net", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "yslither_net", __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__)
#define LOGE(...) printf(__VA_ARGS__)
#endif

void server_init(tenv* env) {
  tuser_data* usr = env->usr;
  game_data* gdata = &usr->gdata;
  user_settings* usrs = &usr->usrs;
  mg_log_set(MG_LL_NONE);
  mg_mgr_init(&gdata->network_manager);
}

void server_connect(tenv* env) {
  tuser_data* usr = env->usr;
  game_data* gdata = &usr->gdata;
  user_settings* usrs = &usr->usrs;

  char url[256] = {};
  sprintf(url, "ws://%s/slither", usrs->ipv4);
  LOGI("server_connect: Attempting WebSocket connection to %s", url);
  gdata->connection =
    mg_ws_connect(&gdata->network_manager, url, server_callback, env,
                  "%s:%s\r\n",
                  "Origin", "https://slither.com");
  if (!gdata->connection) {
    LOGE("server_connect: Failed to allocate connection object!");
  }
}

void server_poll(tenv* env) {
  tuser_data* usr = env->usr;
  game_data* gdata = &usr->gdata;

  mg_mgr_poll(&gdata->network_manager, 0);
}

void server_destroy(tenv* env) {
  tuser_data* usr = env->usr;
  game_data* gdata = &usr->gdata;

  mg_mgr_free(&gdata->network_manager);
}