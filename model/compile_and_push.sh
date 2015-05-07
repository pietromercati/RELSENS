#! /bin/bash

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
 #This file is part of DRM.
 #       DRM is free software: you can redistribute it and/or modify
 #       it under the terms of the GNU General Public License as published by
 #       the Free Software Foundation, either version 3 of the License, or
 #       (at your option) any later version.
 #
 #       DRM is distributed in the hope that it will be useful,
 #       but WITHOUT ANY WARRANTY; without even the implied warranty of
 #       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 #       GNU General Public License for more details.
 #
 #       You should have received a copy of the GNU General Public License
 #       along with DRM.  If not, see <http://www.gnu.org/licenses/>.
 #


#make CC=${NDK_TOOLCHAIN}gcc CFLAGS=--sysroot=${NDK_SYSROOT} relsens_model
/home/pietro/WORK_FOLDER/android-ndk-r10c/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86_64/bin/arm-linux-androideabi-gcc --sysroot=/home/pietro/WORK_FOLDER/android-ndk-r10c/platforms/android-9/arch-arm    relsens_model.c chi_square_density.c ln_gamma_function.c gamma_function.c -lm   -o relsens_model

adb push relsens_model data/PIETRO/

