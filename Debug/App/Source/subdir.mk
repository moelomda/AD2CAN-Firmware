################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/Source/ad2can.c \
../App/Source/ad2can_can.c \
../App/Source/ad2can_hw.c 

OBJS += \
./App/Source/ad2can.o \
./App/Source/ad2can_can.o \
./App/Source/ad2can_hw.o 

C_DEPS += \
./App/Source/ad2can.d \
./App/Source/ad2can_can.d \
./App/Source/ad2can_hw.d 


# Each subdirectory must supply rules for building sources it contributes
App/Source/%.o App/Source/%.su App/Source/%.cyclo: ../App/Source/%.c App/Source/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G473xx -c -I../Core/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I"F:/study/FAU/New folder/High_VoltageTeam/App/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App-2f-Source

clean-App-2f-Source:
	-$(RM) ./App/Source/ad2can.cyclo ./App/Source/ad2can.d ./App/Source/ad2can.o ./App/Source/ad2can.su ./App/Source/ad2can_can.cyclo ./App/Source/ad2can_can.d ./App/Source/ad2can_can.o ./App/Source/ad2can_can.su ./App/Source/ad2can_hw.cyclo ./App/Source/ad2can_hw.d ./App/Source/ad2can_hw.o ./App/Source/ad2can_hw.su

.PHONY: clean-App-2f-Source

