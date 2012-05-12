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

PREBUILT=$ANDROID_NDK_HOME/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86
PLATFORM=$ANDROID_NDK_HOME/platforms/android-$API_VERSION/arch-arm

(
	cd libav

	./configure --target-os=linux \
		--arch=arm \
		--cpu=armv7-a \
		--extra-cflags='-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16' \
		--extra-ldflags='-Wl,--fix-cortex-a8' \
		--cross-prefix=$PREBUILT/bin/arm-linux-androideabi- \
		--sysroot=$PLATFORM \
		--enable-shared \
		--disable-static \
		--enable-version3 \
		--enable-small \
		--disable-protocols \
		--enable-protocol=file,http \
		--disable-demuxers \
		--enable-demuxer=mpegts \
		--disable-filters \
		--disable-debug \
		--disable-doc \
		--disable-ffmpeg \
		--disable-avplay \
		--disable-avconv \
		--disable-avprobe \
		--disable-avserver \
		--disable-avdevice \
		--disable-encoders \
		--disable-muxers \
		--disable-devices \
		--disable-symver || exit 1

	echo "Patch config.h"

	cat << EOF >> config.h
#undef restrict
#define restrict
EOF

	cp -v ../template_libav.mk Android.mk
	cp -v ../template_libav_module.mk libavcodec/Android.mk
	cp -v ../template_libav_module.mk libavfilter/Android.mk
	cp -v ../template_libav_module.mk libavformat/Android.mk
	cp -v ../template_libav_module.mk libavutil/Android.mk
	cp -v ../template_libav_module.mk libswscale/Android.mk
	cp -v ../template_libav_module.mk libpostproc/Android.mk
)
