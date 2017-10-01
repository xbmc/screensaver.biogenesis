project(screensaver.biogenesis)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(Kodi REQUIRED)
if(NOT WIN32)
  find_package(OpenGL REQUIRED)
  set(DEPLIBS ${OPENGL_LIBRARIES})
endif()

include_directories(${KODI_INCLUDE_DIR}/..) # Hack way with "/..", need bigger Kodi cmake rework to match right include ways

set(BIOGENESIS_SOURCES src/Life.cpp)

set(BIOGENESIS_HEADERS src/types.h)

build_addon(screensaver.biogenesis BIOGENESIS DEPLIBS)

include(CPack)
