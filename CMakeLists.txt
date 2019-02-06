project(screensaver.biogenesis)

cmake_minimum_required(VERSION 3.3)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

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

include_directories(${includes} ${KODI_INCLUDE_DIR}/..) # Hack way with "/..", need bigger Kodi cmake rework to match right include ways

set(BIOGENESIS_SOURCES src/Life.cpp)
set(BIOGENESIS_HEADERS src/types.h)

if(NOT WIN32)
  list(APPEND BIOGENESIS_SOURCES src/shaders/GUIShader.cpp
                                 src/shaders/Matrix.cpp
                                 src/shaders/Shader.cpp)
  list(APPEND BIOGENESIS_HEADERS src/shaders/GUIShader.h
                                 src/shaders/Matrix.h
                                 src/shaders/Shader.h)
endif()

build_addon(screensaver.biogenesis BIOGENESIS DEPLIBS)

include(CPack)
