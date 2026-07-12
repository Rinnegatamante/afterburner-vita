cmake_minimum_required(VERSION 3.14)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(FalsoNDK CXX)

include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(FalsoNDK STATIC
    FalsoNDK_Utils.cpp
    android/AAssetManager.cpp
    android/AConfiguration.cpp
    android/AInput.cpp
    android/ALooper.cpp
    android/ANativeActivity.cpp
    android/ANativeWindow.cpp
    android/ASensor.cpp
    linux/fndk_epoll.cpp
    linux/fndk_eventfd.cpp
    linux/fndk_pipe.cpp
    linux/fndk_unistd.cpp
    shim/fndk_controls.cpp
    shim/fndk_sensors.cpp
)

target_include_directories(FalsoNDK PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
