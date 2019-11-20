cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(volume_profile)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib -Wall -Wextra -O2 -std=c99")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")

add_executable(${PROJECT_NAME}
  main.c
  volume.c
  profile.c
)

target_link_libraries(${PROJECT_NAME}
  SceAVConfig_stub
  SceCtrl_stub
  SceIofilemgr_stub
  SceKernelThreadMgr_stub
  SceLibKernel_stub
  ScePafToplevel_stub_weak
  SceRegistryMgr_stub
  SceSysmodule_stub
  taihen_stub
)

vita_create_self(${PROJECT_NAME}.suprx ${PROJECT_NAME} CONFIG exports.yml UNSAFE)
