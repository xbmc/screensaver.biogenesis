cmake_minimum_required(VERSION 3.5)
project(screensaver.biogenesis)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(Kodi REQUIRED)

if(NOT WIN32)
  if(APP_RENDER_SYSTEM STREQUAL "gl" OR NOT APP_RENDER_SYSTEM)
    find_package(OpenGl REQUIRED)
    set(DEPLIBS ${OPENGL_LIBRARIES})
    set(includes ${OPENGL_INCLUDE_DIR})
    add_definitions(${OPENGL_DEFINITIONS})
  elseif(APP_RENDER_SYSTEM STREQUAL "gles")
    find_package(OpenGLES REQUIRED)
    set(DEPLIBS ${OPENGLES_LIBRARIES})
    set(includes ${OPENGLES_INCLUDE_DIR})
    add_definitions(${OPENGLES_DEFINITIONS})
  endif()
endif()

include_directories(${includes} ${KODI_INCLUDE_DIR}/..  # Hack way with "/..", need bigger Kodi cmake rework to match right include ways
                                ${PROJECT_SOURCE_DIR}/lib)

set(BIOGENESIS_SOURCES src/Life.cpp)
set(BIOGENESIS_HEADERS src/types.h)

build_addon(screensaver.biogenesis BIOGENESIS DEPLIBS)

include(CPack)
