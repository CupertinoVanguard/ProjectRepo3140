################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../board/board.c \
../board/clock_config.c \
../board/pin_mux.c 

OBJS += \
./board/board.o \
./board/clock_config.o \
./board/pin_mux.o 

C_DEPS += \
./board/board.d \
./board/clock_config.d \
./board/pin_mux.d 


# Each subdirectory must supply rules for building sources it contributes
board/%.o: ../board/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -DDEBUG -DCPU_MKL25Z128VLK4 -DPRINTF_ADVANCED_ENABLE=1 -DFRDM_KL25Z -DFREEDOM -DCPU_MKL25Z128VLK4_cm0plus -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -D__MCUXPRESSO -D__USE_CMSIS -D__REDLIB__ -I"/Users/nikpil23/Desktop/frdmkl25z_driver_examples_i2c_read_accel_value_transfer/board" -I"/Users/nikpil23/Desktop/frdmkl25z_driver_examples_i2c_read_accel_value_transfer/source" -I"/Users/nikpil23/Desktop/frdmkl25z_driver_examples_i2c_read_accel_value_transfer" -I"/Users/nikpil23/Desktop/frdmkl25z_driver_examples_i2c_read_accel_value_transfer/CMSIS" -I"/Users/nikpil23/Desktop/frdmkl25z_driver_examples_i2c_read_accel_value_transfer/drivers" -I"/Users/nikpil23/Desktop/frdmkl25z_driver_examples_i2c_read_accel_value_transfer/utilities" -I"/Users/nikpil23/Desktop/frdmkl25z_driver_examples_i2c_read_accel_value_transfer/startup" -O0 -fno-common -g -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m0plus -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


