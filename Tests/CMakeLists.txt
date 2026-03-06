cmake_minimum_required(VERSION 3.10)
project(HighVoltageTeam_Tests C)

set(CMAKE_C_STANDARD 11)

# ============================================================================
# Compiler Flags
# ============================================================================
if(MSVC)
    add_compile_options(/W3)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare)
endif()

# ============================================================================
# Include Paths - Mocks take priority to override STM32 headers
# ============================================================================
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Mocks          # Mock headers (main.h, cmsis_os.h, etc.)
    ${CMAKE_CURRENT_SOURCE_DIR}/Unity
    ${CMAKE_CURRENT_SOURCE_DIR}/../App/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../App/Inc/HAL
    ${CMAKE_CURRENT_SOURCE_DIR}/../App/Inc/Drivers
    ${CMAKE_CURRENT_SOURCE_DIR}/../App/Inc/Tasks
)

# ============================================================================
# Mock Libraries (shared across all tests)
# ============================================================================
add_library(mocks STATIC
    Mocks/mock_stm32_hal.c
    Mocks/mock_cmsis_os.c
)

# ============================================================================
# Individual test executables (each self-contained to avoid symbol conflicts)
# ============================================================================

# --- Unit Test: Sensors (pure computation, no HAL dependencies) ---
add_executable(test_sensors
    Unit/test_sensors.c
)
target_link_libraries(test_sensors mocks m)

# --- Unit Test: ADC HAL ---
add_executable(test_adc_hal
    Unit/test_adc_hal.c
)
target_link_libraries(test_adc_hal mocks m)

# --- Unit Test: CAN HAL ---
add_executable(test_can_hal
    Unit/test_can_hal.c
)
target_link_libraries(test_can_hal mocks m)

# --- Unit Test: TIM HAL ---
add_executable(test_tim_hal
    Unit/test_tim_hal.c
)
target_link_libraries(test_tim_hal mocks m)

# --- Unit Test: Sensor Task ---
add_executable(test_sensor_task
    Unit/test_sensor_task.c
)
target_link_libraries(test_sensor_task mocks m)

# --- Unit Test: Brake Light Task ---
add_executable(test_brake_light_task
    Unit/test_brake_light_task.c
)
target_link_libraries(test_brake_light_task mocks m)

# --- Unit Test: CAN RX Task ---
add_executable(test_can_rx_task
    Unit/test_can_rx_task.c
)
target_link_libraries(test_can_rx_task mocks m)

# --- Unit Test: CAN TX Task ---
add_executable(test_can_tx_task
    Unit/test_can_tx_task.c
)
target_link_libraries(test_can_tx_task mocks m)

# --- Unit Test: App Init ---
add_executable(test_app_init
    Unit/test_app_init.c
)
target_link_libraries(test_app_init mocks m)

# --- Integration Tests ---
add_executable(test_integration
    Integration/test_integration.c
)
target_link_libraries(test_integration mocks m)

# --- Acceptance Tests ---
add_executable(test_acceptance
    Acceptance/test_acceptance.c
)
target_link_libraries(test_acceptance mocks m)

# ============================================================================
# CTest Integration
# ============================================================================
enable_testing()

add_test(NAME UnitTest_Sensors       COMMAND test_sensors)
add_test(NAME UnitTest_ADC_HAL       COMMAND test_adc_hal)
add_test(NAME UnitTest_CAN_HAL       COMMAND test_can_hal)
add_test(NAME UnitTest_TIM_HAL       COMMAND test_tim_hal)
add_test(NAME UnitTest_SensorTask    COMMAND test_sensor_task)
add_test(NAME UnitTest_BrakeLightTask COMMAND test_brake_light_task)
add_test(NAME UnitTest_CanRxTask     COMMAND test_can_rx_task)
add_test(NAME UnitTest_CanTxTask     COMMAND test_can_tx_task)
add_test(NAME UnitTest_AppInit       COMMAND test_app_init)
add_test(NAME IntegrationTests       COMMAND test_integration)
add_test(NAME AcceptanceTests        COMMAND test_acceptance)
