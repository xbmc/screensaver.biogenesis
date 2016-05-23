project(screensaver.biogenesis)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(Kodi REQUIRED)

include_directories(${KODI_INCLUDE_DIR})

set(BIOGENESIS_SOURCES src/Life.cpp)

set(DEPLIBS)

build_addon(screensaver.biogenesis BIOGENESIS DEPLIBS)

include(CPack)
