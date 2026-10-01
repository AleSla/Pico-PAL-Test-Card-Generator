cmake_minimum_required(VERSION 3.13)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pico_pal_generator C CXX ASM)
pico_sdk_init()

add_executable(pico_pal_generator main.c)

pico_generate_pio_header(pico_pal_generator ${CMAKE_CURRENT_LIST_DIR}/video.pio)

target_link_libraries(pico_pal_generator pico_stdlib hardware_pio hardware_dma hardware_pll hardware_pwm hardware_clocks pico_multicore)

pico_add_extra_outputs(pico_pal_generator)
