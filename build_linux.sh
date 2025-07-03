#!/bin/bash
git submodule init
git submodule update --init --recursive

# FFmpeg
sudo apt update

sudo apt install -y \
    build-essential \
    libtbb-dev \
    libgl1-mesa-dev \
    libxxf86vm-dev \
    libx11-dev \
    libxcursor-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxi-dev

sudo apt install -y nasm yasm pkg-config \
                libx264-dev libx265-dev libvpx-dev libfdk-aac-dev \
                libmp3lame-dev libopus-dev libass-dev libpostproc-dev

cd external/ffmpeg

./configure \
    --enable-static \
    --disable-shared \
    --prefix=build/ \
    --enable-debug=3 \
    --disable-stripping \
    --disable-optimizations \
    --extra-cflags="-g" \
    --enable-gpl \
    --enable-nonfree \
    --enable-static \
    --disable-shared \
    --disable-programs \
    --disable-doc \
    --enable-protocol=crypto \
    --enable-protocol=rtp \
    --enable-demuxer=rtp \
    --enable-crypto \
    --enable-hwaccels \
    --enable-network \
    --enable-openssl

make
make install

cd ../../


# img-deinterlace
vendor/premake5/premake5a15 gmake2