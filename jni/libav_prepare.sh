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
		--disable-debug \
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
		--extra-ldflags="-march=$APP_ABI" \
		--sysroot=$PLATFORM \
		--disable-asm \
		--enable-neon \
		--extra-ldflags="-Wl,--entry=main,-rpath-link=$PLATFORM/usr/lib -L$PLATFORM/usr/lib -nostdlib -lc"
	
	echo "Patch config.h"
	
	sed -i 's/#define restrict restrict/#define restrict/g' config.h
	
	cat << EOF >> config.h
#undef HAVE_LRINT
#define HAVE_LRINT 1
#undef HAVE_LRINTF
#define HAVE_LRINTF 1
#undef HAVE_ROUND
#define HAVE_ROUND 1
#undef HAVE_TRUNC
#define HAVE_TRUNC 1
EOF
	
	cp -v ../template_av.mk av.mk
	cp -v ../template_libav.mk Android.mk
	cp -v ../template_libavcodec.mk libavcodec/Android.mk
	cp -v ../template_libavfilter.mk libavfilter/Android.mk
	cp -v ../template_libavformat.mk libavformat/Android.mk
	cp -v ../template_libavutil.mk libavutil/Android.mk
	cp -v ../template_libswscale.mk libswscale/Android.mk
	cp -v ../template_libpostproc.mk libpostproc/Android.mk
)
