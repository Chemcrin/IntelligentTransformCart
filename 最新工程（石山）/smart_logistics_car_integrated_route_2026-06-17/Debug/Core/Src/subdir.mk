################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/atk_ms53l1m.c \
../Core/Src/board_app.c \
../Core/Src/chassis_control.c \
../Core/Src/csv_debug.c \
../Core/Src/gpio.c \
../Core/Src/i2c.c \
../Core/Src/imu_app.c \
../Core/Src/jy61p.c \
../Core/Src/laser_app.c \
../Core/Src/main.c \
../Core/Src/mecanum.c \
../Core/Src/mecanum_kinematics.c \
../Core/Src/modbus_rtu.c \
../Core/Src/motor_manager.c \
../Core/Src/sensor_bridge.c \
../Core/Src/sensors.c \
../Core/Src/stm32f1xx_hal_msp.c \
../Core/Src/stm32f1xx_it.c \
../Core/Src/straight_control.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f1xx.c \
../Core/Src/usart.c \
../Core/Src/zdt_x42_modbus.c 

OBJS += \
./Core/Src/atk_ms53l1m.o \
./Core/Src/board_app.o \
./Core/Src/chassis_control.o \
./Core/Src/csv_debug.o \
./Core/Src/gpio.o \
./Core/Src/i2c.o \
./Core/Src/imu_app.o \
./Core/Src/jy61p.o \
./Core/Src/laser_app.o \
./Core/Src/main.o \
./Core/Src/mecanum.o \
./Core/Src/mecanum_kinematics.o \
./Core/Src/modbus_rtu.o \
./Core/Src/motor_manager.o \
./Core/Src/sensor_bridge.o \
./Core/Src/sensors.o \
./Core/Src/stm32f1xx_hal_msp.o \
./Core/Src/stm32f1xx_it.o \
./Core/Src/straight_control.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f1xx.o \
./Core/Src/usart.o \
./Core/Src/zdt_x42_modbus.o 

C_DEPS += \
./Core/Src/atk_ms53l1m.d \
./Core/Src/board_app.d \
./Core/Src/chassis_control.d \
./Core/Src/csv_debug.d \
./Core/Src/gpio.d \
./Core/Src/i2c.d \
./Core/Src/imu_app.d \
./Core/Src/jy61p.d \
./Core/Src/laser_app.d \
./Core/Src/main.d \
./Core/Src/mecanum.d \
./Core/Src/mecanum_kinematics.d \
./Core/Src/modbus_rtu.d \
./Core/Src/motor_manager.d \
./Core/Src/sensor_bridge.d \
./Core/Src/sensors.d \
./Core/Src/stm32f1xx_hal_msp.d \
./Core/Src/stm32f1xx_it.d \
./Core/Src/straight_control.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f1xx.d \
./Core/Src/usart.d \
./Core/Src/zdt_x42_modbus.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../UserModules/remote_cmd -I../UserModules/competition_route -I../Drivers/VL53L1X/core/inc -I../Drivers/VL53L1X/platform/inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/atk_ms53l1m.cyclo ./Core/Src/atk_ms53l1m.d ./Core/Src/atk_ms53l1m.o ./Core/Src/atk_ms53l1m.su ./Core/Src/board_app.cyclo ./Core/Src/board_app.d ./Core/Src/board_app.o ./Core/Src/board_app.su ./Core/Src/chassis_control.cyclo ./Core/Src/chassis_control.d ./Core/Src/chassis_control.o ./Core/Src/chassis_control.su ./Core/Src/csv_debug.cyclo ./Core/Src/csv_debug.d ./Core/Src/csv_debug.o ./Core/Src/csv_debug.su ./Core/Src/gpio.cyclo ./Core/Src/gpio.d ./Core/Src/gpio.o ./Core/Src/gpio.su ./Core/Src/i2c.cyclo ./Core/Src/i2c.d ./Core/Src/i2c.o ./Core/Src/i2c.su ./Core/Src/imu_app.cyclo ./Core/Src/imu_app.d ./Core/Src/imu_app.o ./Core/Src/imu_app.su ./Core/Src/jy61p.cyclo ./Core/Src/jy61p.d ./Core/Src/jy61p.o ./Core/Src/jy61p.su ./Core/Src/laser_app.cyclo ./Core/Src/laser_app.d ./Core/Src/laser_app.o ./Core/Src/laser_app.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/mecanum.cyclo ./Core/Src/mecanum.d ./Core/Src/mecanum.o ./Core/Src/mecanum.su ./Core/Src/mecanum_kinematics.cyclo ./Core/Src/mecanum_kinematics.d ./Core/Src/mecanum_kinematics.o ./Core/Src/mecanum_kinematics.su ./Core/Src/modbus_rtu.cyclo ./Core/Src/modbus_rtu.d ./Core/Src/modbus_rtu.o ./Core/Src/modbus_rtu.su ./Core/Src/motor_manager.cyclo ./Core/Src/motor_manager.d ./Core/Src/motor_manager.o ./Core/Src/motor_manager.su ./Core/Src/sensor_bridge.cyclo ./Core/Src/sensor_bridge.d ./Core/Src/sensor_bridge.o ./Core/Src/sensor_bridge.su ./Core/Src/sensors.cyclo ./Core/Src/sensors.d ./Core/Src/sensors.o ./Core/Src/sensors.su ./Core/Src/stm32f1xx_hal_msp.cyclo ./Core/Src/stm32f1xx_hal_msp.d ./Core/Src/stm32f1xx_hal_msp.o ./Core/Src/stm32f1xx_hal_msp.su ./Core/Src/stm32f1xx_it.cyclo ./Core/Src/stm32f1xx_it.d ./Core/Src/stm32f1xx_it.o ./Core/Src/stm32f1xx_it.su ./Core/Src/straight_control.cyclo ./Core/Src/straight_control.d ./Core/Src/straight_control.o ./Core/Src/straight_control.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f1xx.cyclo ./Core/Src/system_stm32f1xx.d ./Core/Src/system_stm32f1xx.o ./Core/Src/system_stm32f1xx.su ./Core/Src/usart.cyclo ./Core/Src/usart.d ./Core/Src/usart.o ./Core/Src/usart.su ./Core/Src/zdt_x42_modbus.cyclo ./Core/Src/zdt_x42_modbus.d ./Core/Src/zdt_x42_modbus.o ./Core/Src/zdt_x42_modbus.su

.PHONY: clean-Core-2f-Src

