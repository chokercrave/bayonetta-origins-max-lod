cmake_minimum_required(VERSION 3.16)
project(origins_lod NONE)

# The real build is exlaunch's Makefile (devkitA64 + libnx switch_rules, custom specs and linker script). This wrapper
# only drives it, so IDEs and `cmake --build` work: native make when DEVKITPRO is set, the devkitpro/devkita64 Docker
# image otherwise (see build.sh). Output: deploy/subsdk7, packaged in dist/ (no main.npdm: the game's own is used).
find_program(SH sh REQUIRED)

add_custom_target(lod ALL
    COMMAND ${SH} ${CMAKE_CURRENT_SOURCE_DIR}/build.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    USES_TERMINAL
    COMMENT "Building origins-lod (exlaunch subsdk7)")

add_custom_target(lod-clean
    COMMAND ${CMAKE_COMMAND} -E rm -rf build deploy dist work.elf work.nso work.npdm
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
