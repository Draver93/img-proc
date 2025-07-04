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
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy everything into container
COPY . .

# Make the build script executable
RUN chmod +x build_linux.sh

# Run the build script
RUN ./build_linux.sh

# Make img-deinterlace
RUN make

# Drop into bash after build
ENTRYPOINT ["/bin/bash"]