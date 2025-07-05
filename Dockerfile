# Use LF line endings for cross-platform compatibility
FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

# Install all dependencies (add zlib1g-dev, libgl1-mesa-dev, libtbb-dev, libssl-dev etc.)
RUN apt-get update && apt-get install -y \
    build-essential \
    libtbb-dev \
    libgl1-mesa-dev \
    libxxf86vm-dev \
    libx11-dev \
    libxcursor-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxi-dev \
    zlib1g-dev \
    libssl-dev \
    pkg-config \
    nasm yasm \
    libx264-dev libx265-dev libvpx-dev libfdk-aac-dev \
    libmp3lame-dev libopus-dev libass-dev libpostproc-dev \
    git \
    dos2unix \
    xvfb \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy everything into container
COPY . .

# Ensure build script has correct line endings and is executable
RUN dos2unix build_linux.sh 2>/dev/null || true
RUN chmod +x build_linux.sh

# Run the build script
RUN ./build_linux.sh release

# Drop into bash after build
ENTRYPOINT ["/bin/bash"]