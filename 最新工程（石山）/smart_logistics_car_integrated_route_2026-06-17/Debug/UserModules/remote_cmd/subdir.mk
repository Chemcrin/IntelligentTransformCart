################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../UserModules/remote_cmd/remote_cmd.c 

OBJS += \
./UserModules/remote_cmd/remote_cmd.o 

C_DEPS += \
./UserModules/remote_cmd/remote_cmd.d 


# Each subdirectory must supply rules for building sources it contributes
UserModules/remote_cmd/%.o UserModules/remote_cmd/%.su UserModules/remote_cmd/%.cyclo: ../UserModules/remote_cmd/%.c UserModules/remote_cmd/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../UserModules/remote_cmd -I../UserModules/competition_route -I../Drivers/VL53L1X/core/inc -I../Drivers/VL53L1X/platform/inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-UserModules-2f-remote_cmd

clean-UserModules-2f-remote_cmd:
	-$(RM) ./UserModules/remote_cmd/remote_cmd.cyclo ./UserModules/remote_cmd/remote_cmd.d ./UserModules/remote_cmd/remote_cmd.o ./UserModules/remote_cmd/remote_cmd.su

.PHONY: clean-UserModules-2f-remote_cmd

