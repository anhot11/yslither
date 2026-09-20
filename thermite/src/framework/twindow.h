#ifndef TWINDOW_H
#define TWINDOW_H

#ifdef __ANDROID__
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <time.h>

static inline double glfwGetTime(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static inline void glfwSetTime(double t) {
  (void)t;
}
#endif

#include <GLFW/glfw3.h>
#include <cglm/struct.h>
#include <stdbool.h>

typedef struct tkeyboard tkeyboard;
typedef struct tmouse tmouse;
typedef struct twindow twindow;
typedef struct tenv tenv;

typedef void (*trender_func)(tenv* env);
typedef void (*tresize_func)(tenv* env);

typedef struct twindow {
#ifdef __ANDROID__
  struct android_app* app;
  ANativeWindow* a_window;
  bool closed;
#else
  GLFWwindow* handle;
#endif
  ivec2 size;
  ivec2 lsize;
  ivec2 lpos;
  trender_func _render_func;
  tresize_func _resize_func;
  tenv* env;
  bool _refresh;
} twindow;

void twindow_request_refresh(twindow* twindow);
twindow* twindow_create(tenv* env, trender_func render_func,
                        tresize_func resize_func);
void twindow_poll_input(twindow* window);
void twindow_wait_input(twindow* window);
void twindow_toggle_fullscreen(twindow* window);
bool twindow_key_down(twindow* window, int key);
bool twindow_button_down(twindow* window, int button);
bool twindow_closed(twindow* window);
void twindow_destroy(twindow* window);

#endif