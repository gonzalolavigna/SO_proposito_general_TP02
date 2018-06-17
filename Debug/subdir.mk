################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../MessageQueue.c \
../SerialManager.c \
../main.c \
../receiveSerialThread.c \
../rs232.c \
../sendSerialThread.c \
../socketThread.c \
../watchdogThread.c 

OBJS += \
./MessageQueue.o \
./SerialManager.o \
./main.o \
./receiveSerialThread.o \
./rs232.o \
./sendSerialThread.o \
./socketThread.o \
./watchdogThread.o 

C_DEPS += \
./MessageQueue.d \
./SerialManager.d \
./main.d \
./receiveSerialThread.d \
./rs232.d \
./sendSerialThread.d \
./socketThread.d \
./watchdogThread.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


