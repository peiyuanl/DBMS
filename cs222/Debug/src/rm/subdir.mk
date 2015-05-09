################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/rm/rm.cc \
../src/rm/rmtest_create_tables.cc 

OBJS += \
./src/rm/rm.o \
./src/rm/rmtest_create_tables.o 

CC_DEPS += \
./src/rm/rm.d \
./src/rm/rmtest_create_tables.d 


# Each subdirectory must supply rules for building sources it contributes
src/rm/%.o: ../src/rm/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


