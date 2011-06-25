#!/bin/bash

SDL_SRC=SDL-1.2.13.tar.gz

export CFLAGS='-I$ROOTFS_PATH/usr/include'
export CPPFLAGS='-I$ROOTFS_PATH/usr/include'
export LDFLAGS='-L$ROOTFS_PATH/usr/lib'
export CC=arm-linux-uclibcgnueabi-gcc
export CXX=arm-linux-uclibcgnueabi-g++

pushd $PROJECT_PATH/packages/sdl

if [ ! -e $SDL_SRC ]; then
	wget http://www.libsdl.org/release/$SDL_SRC
fi

SDL_DIR=`echo "$SDL_SRC" | cut -d '.' -f -3`

if [ "$CLEAN" == "1" ]; then
	rm -rf $SDL_DIR
	tar -xzf $SDL_SRC
fi

pushd $SDL_DIR

if [ "$CLEAN" == "1" ]; then
	./autogen.sh
fi

./configure --prefix=$ROOTFS_PATH/usr --build=`uname -m` --host=arm-linux-uclibcgnueabi --disable-video-opengl --disable-video-x11 --disable-esd --disable-video-directfb --enable-video-fbcon --enable-pulseaudio=no --enable-input-tslib --enable-rpath=yes --enable-shared

make -j3
make install

popd

popd

exit 0
