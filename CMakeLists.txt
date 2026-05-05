# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")
set(FREERTOS_KERNEL_PATH "/opt/FreeRTOS-Kernel")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
include(FreeRTOS_Kernel_import.cmake)

project(pico2-freertos-tb C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

if (TARGET tinyusb_device)
    message("With TinuUSB build...")
    # Add executable. Default name is the project name, version 0.1

    add_executable(pico2-freertos-tb pico2-freertos-tb.c src/mcp9601_hal.c src/uart_helpers.c src/ina228.c src/pac19xx.c)

    pico_set_program_name(pico2-freertos-tb "pico2-freertos-tb")
    pico_set_program_version(pico2-freertos-tb "0.1")

    # Modify the below lines to enable/disable output over UART/USB
    pico_enable_stdio_usb(pico2-freertos-tb 0)
    pico_enable_stdio_uart(pico2-freertos-tb 1)
    

    # Add the standard library to the build
    target_link_libraries(pico2-freertos-tb
        pico_stdlib
        pico_time
        FreeRTOS-Kernel
        FreeRTOS-Kernel-Heap4
    )

    # Add the standard include files to the build
    target_include_directories(pico2-freertos-tb PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
    )

    # Add any user requested libraries
    target_link_libraries(pico2-freertos-tb 
        hardware_spi
        hardware_i2c
        hardware_dma
        hardware_timer
        hardware_clocks
        hardware_uart
        )

    pico_add_extra_outputs(pico2-freertos-tb)
elseif(PICO_ON_DEVICE)
    message("Skipping hello_usb because TinyUSB submodule is not initialized in the SDK")
endif()
