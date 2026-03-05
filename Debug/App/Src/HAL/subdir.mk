################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/Src/HAL/adc_hal.c \
../App/Src/HAL/can_hal.c \
../App/Src/HAL/tim_hal.c 

OBJS += \
./App/Src/HAL/adc_hal.o \
./App/Src/HAL/can_hal.o \
./App/Src/HAL/tim_hal.o 

C_DEPS += \
./App/Src/HAL/adc_hal.d \
./App/Src/HAL/can_hal.d \
./App/Src/HAL/tim_hal.d 


# Each subdirectory must supply rules for building sources it contributes
App/Src/HAL/%.o App/Src/HAL/%.su App/Src/HAL/%.cyclo: ../App/Src/HAL/%.c App/Src/HAL/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G473xx -c -I../Core/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I"F:/study/FAU/HV/High_VoltageTeam/High_VoltageTeam/App/Inc" -I"F:/study/FAU/HV/High_VoltageTeam/High_VoltageTeam/App/Inc/HAL" -I"F:/study/FAU/HV/High_VoltageTeam/High_VoltageTeam/App/Inc/Tasks" -I"F:/study/FAU/HV/High_VoltageTeam/High_VoltageTeam/App/Inc/Drivers" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App-2f-Src-2f-HAL

clean-App-2f-Src-2f-HAL:
	-$(RM) ./App/Src/HAL/adc_hal.cyclo ./App/Src/HAL/adc_hal.d ./App/Src/HAL/adc_hal.o ./App/Src/HAL/adc_hal.su ./App/Src/HAL/can_hal.cyclo ./App/Src/HAL/can_hal.d ./App/Src/HAL/can_hal.o ./App/Src/HAL/can_hal.su ./App/Src/HAL/tim_hal.cyclo ./App/Src/HAL/tim_hal.d ./App/Src/HAL/tim_hal.o ./App/Src/HAL/tim_hal.su

.PHONY: clean-App-2f-Src-2f-HAL

