# This file is part of JAV 自動オーディオボリューム
# Copyright © 2019, 2020 浅倉麗子

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.13)

set(CMAKE_TOOLCHAIN_FILE "$ENV{DOLCESDK}/share/dolce.toolchain.cmake" CACHE PATH "DolceSDK toolchain file")
set(DOLCESDK_CMAKE_FILE "$ENV{DOLCESDK}/share/dolce.cmake" CACHE PATH "DolceSDK CMake functions file")
include("${DOLCESDK_CMAKE_FILE}" REQUIRED)

project(jav LANGUAGES C)

add_compile_options(
  -Os -std=c99
  -Wall -Wextra -Wdouble-promotion -Wshadow -Wundef
  -fsingle-precision-constant -fno-common -flto
  -ftree-vectorize -funsafe-math-optimizations
)

add_link_options(
  -nostdlib
  --entry=module_start
)

include_directories(
  common
  sce
)

add_subdirectory(kernel_module)

# Build ScePafToplevel stubs

dolce_gen_libs(ScePaf_stubs
  ScePaf.yml
  LIB ScePaf2_stub_weak
)

# Build self

set(ELF "jav.elf")
set(SELF "jav.suprx")
set(MODULE_INFO "exports.yml")

add_executable("${ELF}"
  main.c
  audio.c
  config.c
  common/util.c
)

target_link_libraries("${ELF}"
  jav_kernel

  taihen_stub

  SceAVConfig_stub
  SceBt_stub
  SceIofilemgr_stub
  SceKernelModulemgr_stub
  SceKernelThreadMgr_stub
  SceLibKernel_stub
  ScePaf2_stub_weak
  SceRegistryMgr_stub
  SceSysmodule_stub
)

dolce_create_self("${SELF}"
  "${ELF}"
  CONFIG "${MODULE_INFO}"
)
