#ifdef __ANDROID__

#include <android/log.h>
#include <android_native_app_glue.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "core/tenv.h"
#include "core/tentry.h"
#include "game/touch_input.h"
#include "user.h"

#define LOG_TAG "yslither"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static touch_state g_touch;
static tenv g_env;
static bool g_initialized = false;
static bool g_has_window = false;

static int32_t handle_input(struct android_app* app, AInputEvent* event) {
  (void)app;
  int32_t event_type = AInputEvent_getType(event);

  if (event_type == AINPUT_EVENT_TYPE_MOTION) {
    int32_t action = AMotionEvent_getAction(event);
    int32_t action_masked = action & AMOTION_EVENT_ACTION_MASK;
    int32_t pointer_index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                            AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

    float screen_w = (float)g_env.wnd->size[0];
    float screen_h = (float)g_env.wnd->size[1];

    if (action_masked == AMOTION_EVENT_ACTION_DOWN ||
        action_masked == AMOTION_EVENT_ACTION_POINTER_DOWN) {
      int id = AMotionEvent_getPointerId(event, pointer_index);
      float x = AMotionEvent_getX(event, pointer_index);
      float y = AMotionEvent_getY(event, pointer_index);
      touch_input_down(&g_touch, id, x, y, screen_w, screen_h);
      return 1;
    } else if (action_masked == AMOTION_EVENT_ACTION_MOVE) {
      size_t count = AMotionEvent_getPointerCount(event);
      for (size_t i = 0; i < count; i++) {
        int id = AMotionEvent_getPointerId(event, i);
        float x = AMotionEvent_getX(event, i);
        float y = AMotionEvent_getY(event, i);
        touch_input_move(&g_touch, id, x, y, screen_w, screen_h);
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
          g_env.config.argv = (char**)app;
          g_env.config.argc = 0;
          g_env.config.vsync = true;
          g_env.config.running = true;
          g_env.config.fullscreen = true;
          g_env.config.title = "yslither";
          g_env.usr = malloc(sizeof(tuser_data));

          tlaunch(&g_env);
          g_env.wnd = twindow_create(&g_env, trender, tresize);
          g_env.ctx = tcontext_create(g_env.wnd, g_env.config.vsync, 3);
          tinit(&g_env);
          touch_input_init(&g_touch);

          g_initialized = true;
        } else {
          g_env.wnd->a_window = app->window;
          g_env.wnd->_refresh = true;
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
          tcontext_destroy(g_env.ctx);
          twindow_destroy(g_env.wnd);
          free(g_env.usr);
          g_initialized = false;
        }
        return;
      }
    }

    if (g_has_window && g_initialized && g_env.config.running) {
      if (g_env.ctx->swapchain_ok) {
        // Forward touch inputs to game data
        tuser_data* usr = (tuser_data*)g_env.usr;
        if (usr && g_touch.active) {
          usr->gdata.bot.output.accel = g_touch.boost;
        }

        tinput(&g_env);
        trender(&g_env);
      }
    }
  }
}

#endif
