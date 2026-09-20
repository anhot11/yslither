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
    -DAPP_VERSION="1.0.9"
    -DSETTINGS_VERSION="1.5"
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
    add_definitions(
        -DGLFW_INCLUDE_NONE
        -DGLFW_INCLUDE_VULKAN
    )

    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/glfw/include)
    file(GLOB_RECURSE GLFW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/glfw/src/*.c)

    add_executable(yslither
        ${CMAKE_CURRENT_SOURCE_DIR}/app/src/main.c
        ${THERMITE_SOURCES}
        ${APP_SOURCES}
        ${GLFW_SOURCES}
    )

    find_package(Vulkan REQUIRED)

    target_link_libraries(yslither
        Vulkan::Vulkan
        m
        pthread
        dl
    )
endif()
