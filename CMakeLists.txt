cmake_minimum_required(VERSION 3.22.1)
project(yslither C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/thermite/src
    ${CMAKE_CURRENT_SOURCE_DIR}/thermite/include
    ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/app/src
    ${CMAKE_CURRENT_SOURCE_DIR}/app/src/cimgui
    ${CMAKE_CURRENT_SOURCE_DIR}/app/src/cimgui/imgui
)

# Common compiler definitions
add_definitions(
    -DCGLM_FORCE_DEPTH_ZERO_TO_ONE
    -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS
    -DMG_TLS=MG_TLS_BUILTIN
    -DAPP_VERSION="1.0.37"
    -DSETTINGS_VERSION="2.5"
    -DCIMGUI_NO_EXPORT
)

# Collect sources
file(GLOB_RECURSE THERMITE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/thermite/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/thermite/src/*.cpp
)

file(GLOB_RECURSE APP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/app/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/src/game/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/src/network/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/src/rendering/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/src/ui/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/src/external/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/src/cimgui/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/app/src/imgui_setup.c
)

if(ANDROID)
    list(FILTER APP_SOURCES EXCLUDE REGEX "imgui_impl_glfw\\.cpp$")

    add_definitions(
        -D__ANDROID__
        -DGLFW_INCLUDE_NONE
    )

    # Android Native App Glue
    include_directories(${ANDROID_NDK}/sources/android/native_app_glue)
    set(ANDROID_GLUE_SRC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

    add_library(yslither SHARED
        ${ANDROID_GLUE_SRC}
        ${CMAKE_CURRENT_SOURCE_DIR}/app/src/android_main.c
        ${THERMITE_SOURCES}
        ${APP_SOURCES}
    )

    target_link_libraries(yslither
        android
        log
        vulkan
        m
    )
else()
    # Desktop build
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/glfw/include)
    set(GLFW_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/context.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/init.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/input.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/monitor.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/platform.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/vulkan.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/window.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/egl_context.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/osmesa_context.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/posix_time.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/posix_thread.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/posix_module.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/posix_poll.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/linux_joystick.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/x11_init.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/x11_monitor.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/x11_window.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/xkb_unicode.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/glx_context.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/null_init.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/null_monitor.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/null_window.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/null_joystick.c
    )
    add_library(glfw STATIC ${GLFW_SOURCES})
    target_compile_definitions(glfw PRIVATE _GLFW_X11)
    target_include_directories(glfw PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include)

    add_executable(yslither
        ${CMAKE_CURRENT_SOURCE_DIR}/app/src/main.c
        ${THERMITE_SOURCES}
        ${APP_SOURCES}
    )

    target_compile_definitions(yslither PRIVATE
        GLFW_INCLUDE_NONE
        GLFW_INCLUDE_VULKAN
    )

    find_package(Vulkan REQUIRED)
    find_package(X11 REQUIRED)

    target_link_libraries(yslither
        glfw
        Vulkan::Vulkan
        ${X11_LIBRARIES}
        m
        pthread
        dl
    )
endif()
