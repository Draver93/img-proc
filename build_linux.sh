#!/bin/bash
git submodule init
git submodule update --init --recursive

# FFmpeg
sudo apt update
sudo apt install -y nasm yasm pkg-config \
                libx264-dev libx265-dev libvpx-dev libfdk-aac-dev \
                libmp3lame-dev libopus-dev libass-dev \
                libavdevice-dev libavfilter-dev libavformat-dev \
                libavcodec-dev libswscale-dev libswresample-dev \
                libpostproc-dev

cd external/ffmpeg

./configure --enable-static --disable-shared --prefix=build/

make
mkdir -p ../../lib/ && cp ./build/lib/*.a ../../lib/

cd ../../


# img-deinterlace
vendor/premake5/premake5a15 gmake2