cmake_minimum_required(VERSION 3.13)

project(MY-RP6502-PROJECT C ASM)

add_subdirectory(tools)

add_executable(fanfare)
rp6502_asset(fanfare 0x19800 images/breadboard.bin)
rp6502_asset(fanfare 0x1B800 images/circuitboard.bin)
rp6502_asset(fanfare 0x1D800 images/portable.bin)
rp6502_executable(fanfare
    breadboard.bin.rp6502
    circuitboard.bin.rp6502
    portable.bin.rp6502
)

target_include_directories(fanfare PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_sources(fanfare PRIVATE
    src/ezpsg.c
    src/bitmap_graphics.c
    src/fanfare.c
)
