cmake_minimum_required(VERSION 3.0)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(jav)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib -Wall -Wextra -O3 -std=c99")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")

# kernel plugin

set(K_NAME "${PROJECT_NAME}_kernel")
set(K_SELF "${K_NAME}.skprx")

add_executable("${K_NAME}"
  main_kernel.c
)

target_compile_options("${K_NAME}" PRIVATE -std=gnu99)

target_link_libraries("${K_NAME}"
  SceCpuForDriver_stub
  taihenForKernel_stub
  taihenModuleUtils_stub
)

vita_create_self("${K_SELF}" "${K_NAME}" CONFIG exports_kernel.yml UNSAFE)

# user plugin

set(U_NAME "${PROJECT_NAME}")
set(U_SELF "${U_NAME}.suprx")

add_executable("${U_NAME}"
  main.c
  audio.c
  config.c
  "${K_SELF}.c"
)

add_custom_command(OUTPUT "${K_SELF}.c"
  COMMAND xxd -i "${K_SELF}" "${K_SELF}.c"
  DEPENDS "${K_SELF}"
  COMMENT "Converting ${K_SELF} to ${K_SELF}.c"
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
