
 # 
 # Author: Pietro Mercati
 #
 # email : pimercat@eng.ucsd.edu
 # 
 # If using this code for research purposes, include 
 # references to the following publications
 # 
 # 1) P.Mercati, A. Bartolini, F. Paterna, T. Rosing and L. Benini; A Linux-governor based 
 #    Dynamic Reliability Manager for android mobile devices. DATE 2014.
 # 2) P.Mercati, A. Bartolini, F. Paterna, L. Benini and T. Rosing; An On-line Reliability 
 #    Emulation Framework. EUC 2014
 # 
 #	This file is part of DRM.
 #      DRM is free software: you can redistribute it and/or modify
 #      it under the terms of the GNU General Public License as published by
 #      the Free Software Foundation, either version 3 of the License, or
 #      (at your option) any later version.
 #
 #      DRM is distributed in the hope that it will be useful,
 #      but WITHOUT ANY WARRANTY; without even the implied warranty of
 #      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 #      GNU General Public License for more details.
 #
 #      You should have received a copy of the GNU General Public License
 #      along with DRM.  If not, see <http://www.gnu.org/licenses/>


obj-m := relsens_driver.o

all:
	make ARCH=arm CROSS_COMPILE=/media/pietro/cf739281-9e11-46cb-880b-25e524f90f73/ODROIDXU3/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi- -C /media/pietro/cf739281-9e11-46cb-880b-25e524f90f73/ODROIDXU3/kernel/samsung/exynos5422 M=$(PWD) modules
	#make ARCH=arm CROSS_COMPILE=/media/ftpdrive/Dragonboard/Android/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi- -C /media/ftpdrive/Dragonboard/Android/out/target/product/msm8960/obj/KERNEL_OBJ M=$(PWD) modules

# Path Odroid board
#./prebuilts/gcc/linux-x86/arm/arm-eabi-4.6

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
