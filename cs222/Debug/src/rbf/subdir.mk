################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/rbf/pfm.cc \
../src/rbf/rbfm.cc 

OBJS += \
./src/rbf/pfm.o \
./src/rbf/rbfm.o 

CC_DEPS += \
./src/rbf/pfm.d \
./src/rbf/rbfm.d 


# Each subdirectory must supply rules for building sources it contributes
src/rbf/%.o: ../src/rbf/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


