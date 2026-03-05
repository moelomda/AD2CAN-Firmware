################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/Src/Drivers/sensors.c 

OBJS += \
./App/Src/Drivers/sensors.o 

C_DEPS += \
./App/Src/Drivers/sensors.d 


# Each subdirectory must supply rules for building sources it contributes
App/Src/Drivers/%.o App/Src/Drivers/%.su App/Src/Drivers/%.cyclo: ../App/Src/Drivers/%.c App/Src/Drivers/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G473xx -c -I../Core/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I"F:/study/FAU/HV/High_VoltageTeam/High_VoltageTeam/App/Inc" -I"F:/study/FAU/HV/High_VoltageTeam/High_VoltageTeam/App/Inc/HAL" -I"F:/study/FAU/HV/High_VoltageTeam/High_VoltageTeam/App/Inc/Tasks" -I"F:/study/FAU/HV/High_VoltageTeam/High_VoltageTeam/App/Inc/Drivers" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App-2f-Src-2f-Drivers

clean-App-2f-Src-2f-Drivers:
	-$(RM) ./App/Src/Drivers/sensors.cyclo ./App/Src/Drivers/sensors.d ./App/Src/Drivers/sensors.o ./App/Src/Drivers/sensors.su

.PHONY: clean-App-2f-Src-2f-Drivers

