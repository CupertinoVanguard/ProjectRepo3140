################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include utilities/subdir.mk
-include startup/subdir.mk
-include source/subdir.mk
-include drivers/subdir.mk
-include board/subdir.mk
-include CMSIS/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: frdmkl25z_driver_examples_i2c_read_accel_value_transfer.axf

# Tool invocations
frdmkl25z_driver_examples_i2c_read_accel_value_transfer.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -Xlinker --gc-sections -Xlinker -Map="frdmkl25z_driver_examples_i2c_read_accel_value_transfer.map" -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m0plus -mthumb -T frdmkl25z_driver_examples_i2c_read_accel_value_transfer_Debug.ld -o "frdmkl25z_driver_examples_i2c_read_accel_value_transfer.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) frdmkl25z_driver_examples_i2c_read_accel_value_transfer.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "frdmkl25z_driver_examples_i2c_read_accel_value_transfer.axf"; # arm-none-eabi-objcopy -v -O binary "frdmkl25z_driver_examples_i2c_read_accel_value_transfer.axf" "frdmkl25z_driver_examples_i2c_read_accel_value_transfer.bin" ; # checksum -p MKL25Z128xxx4 -d "frdmkl25z_driver_examples_i2c_read_accel_value_transfer.bin";
	-@echo ' '

.PHONY: all clean dependents post-build

-include ../makefile.targets
