@echo off
REM ============================================================================
REM  High Voltage Team - Test Suite Build & Run Script
REM  Builds and runs all unit, integration, and acceptance tests
REM ============================================================================
echo.
echo ================================================================
echo   HIGH VOLTAGE TEAM - Test Suite Builder
echo ================================================================
echo.

cd /d "%~dp0"

REM ---- Check for GCC ----
where gcc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] GCC not found in PATH!
    echo Please install MinGW-w64 or add GCC to your PATH.
    echo Download: https://www.mingw-w64.org/downloads/
    pause
    exit /b 1
)

REM ---- Create build directory ----
if not exist build mkdir build

echo [BUILD] Compiling mock library...
gcc -c -Wall -Wno-unused-parameter -Wno-unused-variable ^
    -I Mocks -I Unity -I ../App/Inc -I ../App/Inc/HAL -I ../App/Inc/Drivers -I ../App/Inc/Tasks ^
    Mocks/mock_stm32_hal.c -o build/mock_stm32_hal.o

gcc -c -Wall -Wno-unused-parameter -Wno-unused-variable ^
    -I Mocks -I Unity -I ../App/Inc -I ../App/Inc/HAL -I ../App/Inc/Drivers -I ../App/Inc/Tasks ^
    Mocks/mock_cmsis_os.c -o build/mock_cmsis_os.o

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Mock compilation failed!
    pause
    exit /b 1
)

echo [BUILD] Compiling test suites...
set COMMON_FLAGS=-Wall -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare
set COMMON_INCLUDES=-I Mocks -I Unity -I ../App/Inc -I ../App/Inc/HAL -I ../App/Inc/Drivers -I ../App/Inc/Tasks

REM ---- Compile each test as standalone executable ----
set TESTS=test_sensors test_adc_hal test_can_hal test_tim_hal test_sensor_task test_brake_light_task test_can_rx_task test_can_tx_task test_app_init
set TEST_DIRS=Unit Unit Unit Unit Unit Unit Unit Unit Unit

set FAIL=0

REM Unit tests
for %%T in (test_sensors test_adc_hal test_can_hal test_tim_hal test_sensor_task test_brake_light_task test_can_rx_task test_can_tx_task test_app_init) do (
    echo   Compiling %%T...
    gcc %COMMON_FLAGS% %COMMON_INCLUDES% -o build/%%T.exe Unit/%%T.c build/mock_stm32_hal.o build/mock_cmsis_os.o -lm 2>build/%%T_errors.txt
    if %ERRORLEVEL% NEQ 0 (
        echo   [ERROR] %%T compilation failed!
        type build\%%T_errors.txt
        set FAIL=1
    )
)

REM Integration tests
echo   Compiling test_integration...
gcc %COMMON_FLAGS% %COMMON_INCLUDES% -o build/test_integration.exe Integration/test_integration.c build/mock_stm32_hal.o build/mock_cmsis_os.o -lm 2>build/test_integration_errors.txt
if %ERRORLEVEL% NEQ 0 (
    echo   [ERROR] test_integration compilation failed!
    type build\test_integration_errors.txt
    set FAIL=1
)

REM Acceptance tests
echo   Compiling test_acceptance...
gcc %COMMON_FLAGS% %COMMON_INCLUDES% -o build/test_acceptance.exe Acceptance/test_acceptance.c build/mock_stm32_hal.o build/mock_cmsis_os.o -lm 2>build/test_acceptance_errors.txt
if %ERRORLEVEL% NEQ 0 (
    echo   [ERROR] test_acceptance compilation failed!
    type build\test_acceptance_errors.txt
    set FAIL=1
)

echo.
echo ================================================================
echo   RUNNING ALL TESTS
echo ================================================================
echo.

set TOTAL_PASS=0
set TOTAL_FAIL=0

for %%T in (test_sensors test_adc_hal test_can_hal test_tim_hal test_sensor_task test_brake_light_task test_can_rx_task test_can_tx_task test_app_init test_integration test_acceptance) do (
    if exist build\%%T.exe (
        build\%%T.exe
        if %ERRORLEVEL% NEQ 0 (
            set /a TOTAL_FAIL+=1
        ) else (
            set /a TOTAL_PASS+=1
        )
    )
)

echo.
echo ================================================================
if %TOTAL_FAIL% EQU 0 (
    echo   ALL TEST SUITES PASSED! Hardware deployment is safe.
) else (
    echo   WARNING: Some test suites had failures!
)
echo ================================================================
echo.
pause
