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

add_compile_options(
  -std=gnu99
)

# Build self

set(ELF "jav_kernel.elf")
set(SELF "jav_kernel.skprx")
set(MODULE_INFO "exports.yml")
set(SELF_C "jav_kernel.c")
set(SELF_OBJ "jav_kernel")

add_executable("${ELF}"
  main.c
)

target_link_libraries("${ELF}"
  taihenForKernel_stub
  taihenModuleUtils_stub

  SceCpuForDriver_stub
)

dolce_create_self("${SELF}"
  "${ELF}"
  CONFIG "${MODULE_INFO}"
)

# Build library

add_custom_command(
  OUTPUT "${SELF_C}"
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${SELF}"
  COMMAND xxd -i "${SELF}" "${SELF_C}"
  COMMENT "Converting ${SELF} to ${SELF_C}"
)

add_library("${SELF_OBJ}" STATIC
  "${SELF_C}"
)
