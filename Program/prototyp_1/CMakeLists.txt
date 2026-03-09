# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE Debug)

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
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(prototyp_1 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(${PROJECT_NAME} 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LED_Driver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/RGB_Color.cpp
    )

# Generate PIO Header
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/include/ws2812.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(${PROJECT_NAME} 1)
pico_enable_stdio_usb(${PROJECT_NAME} 1)

# Add the standard library to the build
target_link_libraries(${PROJECT_NAME}
        pico_stdlib
        hardware_gpio
        hardware_timer
        hardware_i2c
        hardware_pio
        )

# Add the standard include files to the build
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
)

pico_add_extra_outputs(${PROJECT_NAME})