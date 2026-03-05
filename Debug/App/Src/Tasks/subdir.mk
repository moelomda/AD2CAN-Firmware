################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/Src/Tasks/brake_light_task.c \
../App/Src/Tasks/can_rx_task.c \
../App/Src/Tasks/can_tx_task.c \
../App/Src/Tasks/sensor_task.c 

OBJS += \
./App/Src/Tasks/brake_light_task.o \
./App/Src/Tasks/can_rx_task.o \
./App/Src/Tasks/can_tx_task.o \
./App/Src/Tasks/sensor_task.o 

C_DEPS += \
./App/Src/Tasks/brake_light_task.d \
./App/Src/Tasks/can_rx_task.d \
./App/Src/Tasks/can_tx_task.d \
./App/Src/Tasks/sensor_task.d 


# Each subdirectory must supply rules for building sources it contributes
App/Src/Tasks/%.o App/Src/Tasks/%.su App/Src/Tasks/%.cyclo: ../App/Src/Tasks/%.c App/Src/Tasks/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G473xx -c -I../Core/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I"F:/study/FAU/HV/High_VoltageTeam/High_VoltageTeam/App/Inc" -I"F:/study/FAU/HV/High_VoltageTeam/High_VoltageTeam/App/Inc/HAL" -I"F:/study/FAU/HV/High_VoltageTeam/High_VoltageTeam/App/Inc/Tasks" -I"F:/study/FAU/HV/High_VoltageTeam/High_VoltageTeam/App/Inc/Drivers" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App-2f-Src-2f-Tasks

clean-App-2f-Src-2f-Tasks:
	-$(RM) ./App/Src/Tasks/brake_light_task.cyclo ./App/Src/Tasks/brake_light_task.d ./App/Src/Tasks/brake_light_task.o ./App/Src/Tasks/brake_light_task.su ./App/Src/Tasks/can_rx_task.cyclo ./App/Src/Tasks/can_rx_task.d ./App/Src/Tasks/can_rx_task.o ./App/Src/Tasks/can_rx_task.su ./App/Src/Tasks/can_tx_task.cyclo ./App/Src/Tasks/can_tx_task.d ./App/Src/Tasks/can_tx_task.o ./App/Src/Tasks/can_tx_task.su ./App/Src/Tasks/sensor_task.cyclo ./App/Src/Tasks/sensor_task.d ./App/Src/Tasks/sensor_task.o ./App/Src/Tasks/sensor_task.su

.PHONY: clean-App-2f-Src-2f-Tasks

