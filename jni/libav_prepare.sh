#!/bin/bash

. ./libav_clean.sh

(
	cd ..
	git submodule init
	git submodule update
)

if [ "$ANDROID_NDK_HOME" == "" ]; then
	echo "ERROR: environment variable ANDROID_NDK_HOME is empty"
	exit 1
fi

API_VERSION=8
#APP_ABI=armv5
APP_ABI=armv7-a

PREBUILT=$ANDROID_NDK_HOME/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86
PLATFORM=$ANDROID_NDK_HOME/platforms/android-$API_VERSION/arch-arm
GCC_ARMLIB=$PREBUILT/lib/gcc/arm-linux-androideabi/4.4.3

(
	cd libav
	
	./configure --target-os=linux \
		--prefix=. \
		--arch=arm \
		--enable-version3 \
		--enable-gpl \
		--disable-protocols \
		--enable-protocol=file \
		--disable-demuxers \
		--enable-demuxer=mpegts \
		--enable-avfilter \
		--disable-ffmpeg \
		--disable-avplay \
		--disable-avconv \
		--disable-avprobe \
		--disable-avserver \
		--disable-avdevice \
		--disable-encoders \
		--disable-muxers \
		--disable-devices \
		--disable-network \
		--enable-cross-compile \
		--cross-prefix=$PREBUILT/bin/arm-linux-androideabi- \
		--extra-cflags="-fPIC -DANDROID -march=$APP_ABI" \
		--sysroot=$PLATFORM \
		--disable-asm \
		--enable-neon \
		--enable-armvfp \
		--extra-ldflags="-Wl,-T,$PREBUILT/arm-linux-androideabi/lib/ldscripts/armelf_linux_eabi.x -Wl,-rpath-link=$PLATFORM/usr/lib -L$PLATFORM/usr/lib -nostdlib $GCC_ARMLIB/crtbegin.o $GCC_ARMLIB/crtend.o -lc -lm -ldl"
	
	sed -i 's/#define restrict restrict/#define restrict/g' config.h
	echo > libavutil/libm.h
	
	cp -v ../template_av.mk av.mk
	cp -v ../template_libav.mk Android.mk
	cp -v ../template_libavcodec.mk libavcodec/Android.mk
	cp -v ../template_libavfilter.mk libavfilter/Android.mk
	cp -v ../template_libavformat.mk libavformat/Android.mk
	cp -v ../template_libavutil.mk libavutil/Android.mk
	cp -v ../template_libswscale.mk libavutil/Android.mk
)
