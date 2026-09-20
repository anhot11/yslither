#ifdef __ANDROID__

#include <android/asset_manager.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "core/tenv.h"
#include "core/tentry.h"
#include "game/touch_input.h"
#include "imgui_setup.h"
#include "user.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"

#define LOG_TAG "yslither"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static touch_state g_touch;
static tenv g_env;
static bool g_initialized = false;
static bool g_has_window = false;

static const char* const ASSET_FILES[] = {
    "app/res/fonts/iconfont.ttf",
    "app/res/fonts/mono_bold.ttf",
    "app/res/fonts/mono_italic.ttf",
    "app/res/fonts/mono_regular.ttf",
    "app/res/fonts/regular_bold.ttf",
    "app/res/fonts/regular_italic.ttf",
    "app/res/fonts/regular_regular.ttf",
    "app/res/textures/background_4k.png",
    "app/res/textures/tex_atlas_8k.png",
    "app/res/shaders/bin/bdf.spv",
    "app/res/shaders/bin/bdv.spv",
    "app/res/shaders/bin/bgf.spv",
    "app/res/shaders/bin/bgv.spv",
    "app/res/shaders/bin/bpf.spv",
    "app/res/shaders/bin/bpv.spv",
    "app/res/shaders/bin/bstf.spv",
    "app/res/shaders/bin/bstv.spv",
    "app/res/shaders/bin/fdf.spv",
    "app/res/shaders/bin/fdv.spv",
    "app/res/shaders/bin/fdrf.spv",
    "app/res/shaders/bin/fdrv.spv",
    "app/res/shaders/bin/mmf.spv",
    "app/res/shaders/bin/mmv.spv",
    "app/res/shaders/bin/sprf.spv",
    "app/res/shaders/bin/sprv.spv",
    NULL
};

static void make_dir_p(const char* dir) {
  char tmp[512];
  snprintf(tmp, sizeof(tmp), "%s", dir);
  size_t len = strlen(tmp);
  if (len > 0 && tmp[len - 1] == '/') tmp[len - 1] = '\0';
  for (char* p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      mkdir(tmp, 0755);
      *p = '/';
    }
  }
  mkdir(tmp, 0755);
}

static void extract_asset(AAssetManager* mgr, const char* asset_path, const char* dest_path) {
  if (!mgr) return;

  AAsset* asset = AAssetManager_open(mgr, asset_path, AASSET_MODE_BUFFER);
  if (!asset) {
    LOGE("Could not open asset: %s", asset_path);
    return;
  }

  off_t asset_size = AAsset_getLength(asset);

  struct stat st;
  if (stat(dest_path, &st) == 0 && st.st_size == asset_size) {
    // File already extracted and sizes match
    AAsset_close(asset);
    return;
  }

  char dir[512];
  snprintf(dir, sizeof(dir), "%s", dest_path);
  char* last_slash = strrchr(dir, '/');
  if (last_slash) {
    *last_slash = '\0';
    make_dir_p(dir);
  }

  FILE* out = fopen(dest_path, "wb");
  if (!out) {
    LOGE("Failed to create file for extraction: %s", dest_path);
    AAsset_close(asset);
    return;
  }

  const void* buffer = AAsset_getBuffer(asset);
  if (buffer) {
    fwrite(buffer, 1, asset_size, out);
  } else {
    char chunk[8192];
    int read_bytes;
    while ((read_bytes = AAsset_read(asset, chunk, sizeof(chunk))) > 0) {
      fwrite(chunk, 1, read_bytes, out);
    }
  }

  fclose(out);
  AAsset_close(asset);
  LOGI("Extracted: %s (%ld bytes)", dest_path, (long)asset_size);
}

static void extract_all_assets(AAssetManager* mgr) {
  LOGI("Checking and extracting game assets...");
  for (int i = 0; ASSET_FILES[i] != NULL; i++) {
    extract_asset(mgr, ASSET_FILES[i], ASSET_FILES[i]);
  }
  LOGI("Asset extraction check complete.");
}

static int32_t handle_input(struct android_app* app, AInputEvent* event) {
  (void)app;
  int32_t event_type = AInputEvent_getType(event);

  if (event_type == AINPUT_EVENT_TYPE_MOTION) {
    int32_t action = AMotionEvent_getAction(event);
    int32_t action_masked = action & AMOTION_EVENT_ACTION_MASK;
    int32_t pointer_index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                            AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

    float screen_w = g_env.wnd ? (float)g_env.wnd->size[0] : 1920.0f;
    float screen_h = g_env.wnd ? (float)g_env.wnd->size[1] : 1080.0f;

    float x = AMotionEvent_getX(event, pointer_index);
    float y = AMotionEvent_getY(event, pointer_index);

    // Forward primary touch to ImGui UI
    ImGuiIO* io = igGetIO_Nil();
    if (io) {
      io->MousePos = (ImVec2){x, y};
      if (action_masked == AMOTION_EVENT_ACTION_DOWN ||
          action_masked == AMOTION_EVENT_ACTION_POINTER_DOWN) {
        io->MouseDown[0] = true;
      } else if (action_masked == AMOTION_EVENT_ACTION_UP ||
                 action_masked == AMOTION_EVENT_ACTION_POINTER_UP ||
                 action_masked == AMOTION_EVENT_ACTION_CANCEL) {
        io->MouseDown[0] = false;
      }
    }

    if (action_masked == AMOTION_EVENT_ACTION_DOWN ||
        action_masked == AMOTION_EVENT_ACTION_POINTER_DOWN) {
      int id = AMotionEvent_getPointerId(event, pointer_index);
      touch_input_down(&g_touch, id, x, y, screen_w, screen_h);
      return 1;
    } else if (action_masked == AMOTION_EVENT_ACTION_MOVE) {
      size_t count = AMotionEvent_getPointerCount(event);
      for (size_t i = 0; i < count; i++) {
        int id = AMotionEvent_getPointerId(event, i);
        float px = AMotionEvent_getX(event, i);
        float py = AMotionEvent_getY(event, i);
        touch_input_move(&g_touch, id, px, py, screen_w, screen_h);
      }
      return 1;
    } else if (action_masked == AMOTION_EVENT_ACTION_UP ||
               action_masked == AMOTION_EVENT_ACTION_POINTER_UP ||
               action_masked == AMOTION_EVENT_ACTION_CANCEL) {
      int id = AMotionEvent_getPointerId(event, pointer_index);
      touch_input_up(&g_touch, id);
      return 1;
    }
  }

  return 0;
}

static void handle_cmd(struct android_app* app, int32_t cmd) {
  switch (cmd) {
    case APP_CMD_INIT_WINDOW:
      LOGI("APP_CMD_INIT_WINDOW");
      if (app->window != NULL) {
        g_has_window = true;
        if (!g_initialized) {
          if (app->activity && app->activity->assetManager) {
            extract_all_assets(app->activity->assetManager);
          }

          g_env.config.argv = (char**)app;
          g_env.config.argc = 0;
          g_env.config.vsync = true;
          g_env.config.running = true;
          g_env.config.fullscreen = true;
          g_env.config.title = "yslither";
          g_env.usr = malloc(sizeof(tuser_data));

          tlaunch(&g_env);
          g_env.wnd = twindow_create(&g_env, trender, tresize);
          g_env.kb = tkeyboard_create(g_env.wnd);
          g_env.ms = tmouse_create(g_env.wnd);
          g_env.ctx = tcontext_create(g_env.wnd, g_env.config.vsync, 3);
          if (!g_env.ctx) {
            LOGE("FATAL: Failed to create Vulkan context!");
            return;
          }

          tinit(&g_env);
          touch_input_init(&g_touch);

          g_initialized = true;
          LOGI("yslither game initialized successfully.");
        } else {
          if (g_env.wnd) {
            g_env.wnd->a_window = app->window;
            g_env.wnd->_refresh = true;
          }
        }
      }
      break;

    case APP_CMD_TERM_WINDOW:
      LOGI("APP_CMD_TERM_WINDOW");
      g_has_window = false;
      if (g_env.wnd) {
        g_env.wnd->a_window = NULL;
      }
      break;

    case APP_CMD_DESTROY:
      LOGI("APP_CMD_DESTROY");
      g_env.config.running = false;
      break;
  }
}

void android_main(struct android_app* state) {
  LOGI("yslither android_main started");

  if (state->activity && state->activity->internalDataPath) {
    chdir(state->activity->internalDataPath);
    LOGI("Working directory set to: %s", state->activity->internalDataPath);
  }

  state->onAppCmd = handle_cmd;
  state->onInputEvent = handle_input;

  while (1) {
    int ident;
    int events;
    struct android_poll_source* source;

    while ((ident = ALooper_pollOnce(g_has_window ? 0 : -1, NULL, &events,
                                     (void**)&source)) >= 0) {
      if (source != NULL) {
        source->process(state, source);
      }

      if (state->destroyRequested != 0) {
        LOGI("Destroy requested, exiting main loop");
        if (g_initialized) {
          tdestroy(&g_env);
          if (g_env.ctx) tcontext_destroy(g_env.ctx);
          if (g_env.ms) tmouse_destroy(g_env.ms);
          if (g_env.kb) tkeyboard_destroy(g_env.kb);
          if (g_env.wnd) twindow_destroy(g_env.wnd);
          free(g_env.usr);
          g_initialized = false;
        }
        return;
      }
    }

    if (g_has_window && g_initialized && g_env.config.running) {
      if (g_env.ctx && g_env.ctx->swapchain_ok) {
        tuser_data* usr = (tuser_data*)g_env.usr;
        if (usr && g_touch.active) {
          usr->gdata.bot.output.accel = g_touch.boost;
          if (g_env.ms) {
            g_env.ms->pos[0] = (float)g_env.ctx->size[0] / 2.0f + g_touch.target_x;
            g_env.ms->pos[1] = (float)g_env.ctx->size[1] / 2.0f + g_touch.target_y;
          }
        }

        tinput(&g_env);
        trender(&g_env);

        if (g_env.kb) tkeyboard_update(g_env.kb);
        if (g_env.ms) tmouse_update(g_env.ms);
      }
    }
  }
}

#endif
