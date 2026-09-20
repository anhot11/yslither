#include "../core/tenv.h"

#include "../util/tdarray.h"
#include "twindow.h"

#ifndef __ANDROID__
void key_callback(GLFWwindow* window, int key, int scancode, int action,
                  int mods) {
  (void)scancode; (void)mods;
  tenv* env = glfwGetWindowUserPointer(window);
  if (action == GLFW_PRESS)
    tdarray_push(&env->kb->keys_pressed, &key);
  else if (action == GLFW_RELEASE)
    tdarray_push(&env->kb->keys_released, &key);
}

void char_callback(GLFWwindow* window, unsigned int codepoint) {
  tenv* env = glfwGetWindowUserPointer(window);
  env->kb->char_pressed = (char)codepoint;
}
#endif

tkeyboard* tkeyboard_create(twindow* window) {
#ifndef __ANDROID__
  glfwSetKeyCallback(window->handle, key_callback);
  glfwSetCharCallback(window->handle, char_callback);
#else
  (void)window;
#endif

  tkeyboard* r = malloc(sizeof(tkeyboard));
  r->keys_pressed = tdarray_create(int);
  r->keys_released = tdarray_create(int);

  return r;
}

void tkeyboard_update(tkeyboard* keyboard) {
  if (!keyboard) return;
  tdarray_clear(keyboard->keys_pressed);
  tdarray_clear(keyboard->keys_released);
  keyboard->char_pressed = 0;
}

int tkeyboard_key_pressed(tkeyboard* keyboard, int key) {
  if (!keyboard || !keyboard->keys_pressed) return 0;
  return tdarray_find(keyboard->keys_pressed, &key) != -1;
}

int tkeyboard_key_released(tkeyboard* keyboard, int key) {
  if (!keyboard || !keyboard->keys_released) return 0;
  return tdarray_find(keyboard->keys_released, &key) != -1;
}

void tkeyboard_destroy(tkeyboard* keyboard) {
  if (!keyboard) return;
  tdarray_destroy(keyboard->keys_released);
  tdarray_destroy(keyboard->keys_pressed);
  free(keyboard);
}