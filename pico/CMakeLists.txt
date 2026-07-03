cmake_minimum_required(VERSION 3.13...3.27)

set(PROJECT_TYPE "HOST" CACHE STRING "Target platform: HOST or PICO")
set_property(CACHE PROJECT_TYPE PROPERTY STRINGS "HOST" "PICO")

if(PROJECT_TYPE STREQUAL "PICO")
    # initialize the SDK based on PICO_SDK_PATH
    # note: this must happen before project()
    include(pico_sdk_import.cmake)
endif()

project(my_project)

if(PROJECT_TYPE STREQUAL "PICO")
    # initialize the Raspberry Pi Pico SDK
    pico_sdk_init()
endif()

add_executable(hi
    main.c
)

target_include_directories(hi PUBLIC ${CMAKE_CURRENT_LIST_DIR})

if(PROJECT_TYPE STREQUAL "PICO")
    target_link_libraries(hi
        pico_stdlib
        tinyusb_host
    )
    pico_enable_stdio_usb(hi 0)
    pico_enable_stdio_uart(hi 1)
    pico_add_extra_outputs(hi)
endif()
