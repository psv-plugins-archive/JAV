set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib -Wall -Wextra -O3 -std=gnu99")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")

set(KJAV     "jav_kernel")
set(ELF      "${KJAV}.elf")
set(SELF     "${KJAV}.skprx")
set(SELF_C   "${KJAV}.c")
set(SELF_OBJ "${KJAV}")

add_executable("${ELF}"
  main.c
)

target_include_directories("${ELF}"
  PRIVATE "${CMAKE_SOURCE_DIR}/common"
)

target_link_libraries("${ELF}"
  SceCpuForDriver_stub
  taihenForKernel_stub
  taihenModuleUtils_stub
)

vita_create_self("${SELF}"
  "${ELF}"
  CONFIG exports.yml
  UNSAFE
)

add_custom_target("${SELF_C}.target"
  DEPENDS "${SELF_C}"
)

add_custom_command(
  OUTPUT "${SELF_C}"
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${SELF}"
  COMMAND xxd -i "${SELF}" "${SELF_C}"
  COMMENT "Converting ${SELF} to ${SELF_C}"
)

add_library("${SELF_OBJ}"
  STATIC
  "${SELF_C}"
)
