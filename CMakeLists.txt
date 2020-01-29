cmake_minimum_required(VERSION 3.0)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(jav LANGUAGES C)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib -Wall -Wextra -O3 -std=c99")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")

add_subdirectory(kernel_module)
add_subdirectory(libpaf)

set(ELF  "${PROJECT_NAME}.elf")
set(SELF "${PROJECT_NAME}.suprx")

add_executable("${ELF}"
  main.c
  audio.c
  config.c
  common/util.c
)

target_include_directories("${ELF}"
  PRIVATE common
  PRIVATE sce
)

target_link_libraries("${ELF}"
  gcc
  SceAVConfig_stub
  SceBt_stub
  SceIofilemgr_stub
  SceKernelModulemgr_stub
  SceKernelThreadMgr_stub
  SceLibKernel_stub
  ScePafToplevel_stub_weak
  SceRegistryMgr_stub
  SceSysmodule_stub
  taihen_stub
  jav_kernel
)

vita_create_self("${SELF}"
  "${ELF}"
  CONFIG exports.yml
  UNSAFE
)
