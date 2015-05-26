################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/ix/ix.cc \
../src/ix/ixtest7.cc \
../src/ix/ixtest_util.cc 

OBJS += \
./src/ix/ix.o \
./src/ix/ixtest7.o \
./src/ix/ixtest_util.o 

CC_DEPS += \
./src/ix/ix.d \
./src/ix/ixtest7.d \
./src/ix/ixtest_util.d 


# Each subdirectory must supply rules for building sources it contributes
src/ix/%.o: ../src/ix/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


