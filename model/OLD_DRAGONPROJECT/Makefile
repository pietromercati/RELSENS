# Paths and settings
TARGET_PRODUCT = msm8960
ANDROID_ROOT = /media/ftpdrive/Dragonboard/Android
BIONIC_LIBC = $(ANDROID_ROOT)/bionic/libc
BIONIC_LIBM = $(ANDROID_ROOT)/bionic/libm
PRODUCT_OUT = $(ANDROID_ROOT)/out/target/product/$(TARGET_PRODUCT)
CROSS_COMPILE = $(ANDROID_ROOT)/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-
# Tool names
AS = $(CROSS_COMPILE)as
AR = $(CROSS_COMPILE)ar
CC = $(CROSS_COMPILE)gcc
CPP = $(CC) -E
LD = $(CROSS_COMPILE)ld
NM = $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
RANLIB = $(CROSS_COMPILE)ranlib
READELF = $(CROSS_COMPILE)readelf
SIZE = $(CROSS_COMPILE)size
STRINGS = $(CROSS_COMPILE)strings
STRIP = $(CROSS_COMPILE)strip
export AS AR CC CPP LD NM OBJCOPY OBJDUMP RANLIB READELF \
SIZE STRINGS STRIP

INCLUDES = -I$(ANDROID_ROOT)/system/core/include            \
           -I$(ANDROID_ROOT)/hardware/libhardware/include        \
           -I$(ANDROID_ROOT)/out/target/product/generic/obj/include    \
           -I$(ANDROID_ROOT)/bionic/libc/arch-arm/include        \
           -I$(ANDROID_ROOT)/bionic/libc/include            \
           -I$(ANDROID_ROOT)/bionic/libstdc++/include        \
           -I$(ANDROID_ROOT)/bionic/libc/kernel/common        \
           -I$(ANDROID_ROOT)/bionic/libc/kernel/arch-arm        \
           -I$(ANDROID_ROOT)/bionic/libm/include            \
           -I$(ANDROID_ROOT)/bionic/libm/include/arch/arm        \
           -I$(ANDROID_ROOT)/bionic/libthread_db/include        \
           -I$(ANDROID_ROOT)/bionic/libm/arm                \
           -I$(ANDROID_ROOT)/bionic/libm

CFLAGS   = -nostdlib      -std=c99                                              \
           $(INCLUDES)                                                  \
           -D__ARM_ARCH_5__ -D__ARM_ARCH_5T__                           \
           -D__ARM_ARCH_5E__ -D__ARM_ARCH_5TE__ -DANDROID               \
           -DSK_RELEASE -DNDEBUG -UDEBUG                                \
           -march=armv5te -mtune=xscale -msoft-float -mthumb-interwork  \
           -fpic -fno-exceptions -ffunction-sections -funwind-tables    \
           -fstack-protector -fmessage-length=0                         \
           -include $(ANDROID_ROOT)/system/core/include/arch/linux-arm/AndroidConfig.h

LDFLAGS = -nostdlib -Wl,-dynamic-linker,/system/bin/linker \
	$(PRODUCT_OUT)/obj/lib/crtbegin_dynamic.o \
	$(PRODUCT_OUT)/obj/lib/crtend_android.o \
	-L$(PRODUCT_OUT)/obj/lib -lm -lc -ldl

 
#CFLAGS = 
#-O2 $(CFLAGS) -Wall -fno-short-enums

# Installation variables
EXEC_NAME = LTC
INSTALL = install
INSTALL_DIR = $(PRODUCT_OUT)/system/bin
# Files needed for the build
OBJS 	= LTC.o chi_square_density.o ln_gamma_function.o gamma_function.o
# Make rules
all: LTC
.c.o:
	$(CC) $(CFLAGS) -c $<

LTC: ${OBJS}
	$(CC) -o $(EXEC_NAME) ${OBJS} $(LDFLAGS)

install: LTC
	test -d $(INSTALL_DIR) || $(INSTALL) -d -m 755 $(INSTALL_DIR)
	$(INSTALL) -m 755 $(EXEC_NAME) $(INSTALL_DIR)

clean:
	rm -f *.o $(EXEC_NAME) core

distclean:
	rm -f *~
	rm -f *.o $(EXEC_NAME) core


