cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(jav)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib -Wall -Wextra -O2 -std=c99")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")

set(U_NAME "${PROJECT_NAME}")
set(U_SELF "${U_NAME}.suprx")

add_executable("${U_NAME}"
  main.c
  audio.c
  config.c
)

target_link_libraries("${U_NAME}"
  SceAVConfig_stub
  SceBt_stub
  SceIofilemgr_stub
  SceKernelThreadMgr_stub
  SceLibKernel_stub
  ScePafToplevel_stub_weak
  SceRegistryMgr_stub
  SceSysmodule_stub
  taihen_stub
)

vita_create_self("${U_SELF}" "${U_NAME}" CONFIG exports.yml UNSAFE)
