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
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(controle_agro C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(controle_agro 
                joystick/joystick.c
                inc/ssd1306_i2c.c 
                display/display.c 
                client/client.c
                temperature/temperature.c
                controle_agro.c 
                button/button.c
                soilHumidity/soilHumidity.c
                motor/motor.c
            )

pico_set_program_name(controle_agro "controle_agro")
pico_set_program_version(controle_agro "0.1")

# Generate PIO header
# pico_generate_pio_header(controle_agro ${CMAKE_CURRENT_LIST_DIR}/motor/ws2818b.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(controle_agro 0)
pico_enable_stdio_usb(controle_agro 1)

# Add the standard library to the build
target_link_libraries(controle_agro
        pico_stdlib 
        hardware_adc 
        hardware_pwm 
        hardware_clocks
        hardware_i2c
    )

# Add the standard include files to the build
target_include_directories(controle_agro PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(controle_agro 
        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_mbedtls
        pico_mbedtls   
        hardware_pio
        hardware_clocks
        )

pico_add_extra_outputs(controle_agro)

