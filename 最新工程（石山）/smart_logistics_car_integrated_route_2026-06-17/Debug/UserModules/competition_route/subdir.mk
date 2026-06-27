################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../UserModules/competition_route/competition_route.c 

OBJS += \
./UserModules/competition_route/competition_route.o 

C_DEPS += \
./UserModules/competition_route/competition_route.d 


# Each subdirectory must supply rules for building sources it contributes
UserModules/competition_route/%.o UserModules/competition_route/%.su UserModules/competition_route/%.cyclo: ../UserModules/competition_route/%.c UserModules/competition_route/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../UserModules/remote_cmd -I../UserModules/competition_route -I../Drivers/VL53L1X/core/inc -I../Drivers/VL53L1X/platform/inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-UserModules-2f-competition_route

clean-UserModules-2f-competition_route:
	-$(RM) ./UserModules/competition_route/competition_route.cyclo ./UserModules/competition_route/competition_route.d ./UserModules/competition_route/competition_route.o ./UserModules/competition_route/competition_route.su

.PHONY: clean-UserModules-2f-competition_route

