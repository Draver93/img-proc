#!/bin/bash
# =============================================================================
# 
# Linux Build Script for Image Deinterlace Tool
# =============================================
# 
# Automated build script for compiling the image deinterlace tool on Linux
# systems. Handles FFmpeg compilation and project building.
# 
# Usage:
#   ./build_linux.sh [debug|release] [clean]
#   - First argument: build type (default: release)
#   - Second argument: 'clean' to force a clean FFmpeg build
# 
# Author: Finoshkin Aleksei
# License: MIT
# 
# =============================================================================

git submodule init
git submodule update --init --recursive

# Accept build configuration as an argument (default: release)
BUILD_TYPE=${1:-release}
CLEAN_FFMPEG=${2:-}

# FFmpeg
apt-get update

apt-get install -y \
    build-essential \
    libtbb-dev \
    libgl1-mesa-dev \
    libxxf86vm-dev \
    libx11-dev \
    libxcursor-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxi-dev

apt-get install -y nasm yasm pkg-config \
                libx264-dev libx265-dev libvpx-dev libfdk-aac-dev \
                libmp3lame-dev libopus-dev libass-dev libpostproc-dev \
                libssl-dev

cd external/ffmpeg

if [ "$CLEAN_FFMPEG" == "clean" ]; then
    make distclean || true
fi

# Set FFmpeg configure flags based on build type
if [ "$BUILD_TYPE" == "release" ]; then
    FFMPEG_CONFIG_FLAGS="\
        --disable-debug \
        --enable-optimizations \
        --enable-stripping \
        --extra-cflags=-O3 \
        --enable-gpl \
        --enable-nonfree \
        --enable-static \
        --disable-shared \
        --disable-programs \
        --disable-doc \
        --enable-protocol=crypto \
        --enable-protocol=rtp \
        --enable-demuxer=rtp \
        --enable-hwaccels \
        --enable-network \
        --enable-openssl"
else
    FFMPEG_CONFIG_FLAGS="\
        --enable-debug=3 \
        --disable-stripping \
        --disable-optimizations \
        --extra-cflags=-g \
        --enable-gpl \
        --enable-nonfree \
        --enable-static \
        --disable-shared \
        --disable-programs \
        --disable-doc \
        --enable-protocol=crypto \
        --enable-protocol=rtp \
        --enable-demuxer=rtp \
        --enable-hwaccels \
        --enable-network \
        --enable-openssl"
fi

./configure --prefix=build/ $FFMPEG_CONFIG_FLAGS
make
make install

cd ../../

# img-deinterlace
vendor/premake5/premake5a15 gmake2
make config=$BUILD_TYPE