# Image Deinterlace Tool

A high-performance C++ tool for deinterlacing interlaced images using multiple processing modes including CPU, GPU, and multi-threaded approaches.

## Features

- **Multiple Processing Modes**: CPU, GPU (OpenGL), async, multi-threaded, and SIMD deinterlacing
- **FFmpeg Integration**: Built on FFmpeg for robust image format support
- **Linux Support**: Optimized for Linux systems
- **High Performance**: GPU acceleration with OpenGL compute shaders and SIMD vectorization
- **Pipeline Architecture**: Modular design for easy extension

## Quick Start

### Prerequisites

- **Linux**: GCC 7+ or Clang 8+
- **FFmpeg**: Built from source (included in external/ffmpeg/)
- **OpenGL**: 4.3+ for GPU mode (optional)
- **CPU**: AVX2 support for SIMD mode (optional)

### Building

```bash
# Linux
./build_linux.sh
```

**Note**: The build script will automatically compile FFmpeg from source in the `external/ffmpeg/` directory.

### Docker Support

A `Dockerfile` is provided for easy building and running of the project in a containerized environment. This is useful if you want to avoid installing dependencies directly on your system.

### Build and Run with Docker

```bash
# Build the Docker image
sudo docker build -t img-deinterlace .

# Start a container and open a shell
sudo docker run -it --rm img-deinterlace

# Inside the container, you can run:
./bin/Debug-linux-x86_64/img_deinterlace/img_deinterlace --help
```

You can also mount a local directory to the container to process your own images:

```bash
sudo docker run -it --rm -v /path/to/images:/data img-deinterlace \
  ./bin/Debug-linux-x86_64/img_deinterlace/img_deinterlace --input /data/in.jpg --output /data/out.jpg
```

**Note**: For GPU mode in Docker, you'll need to start a virtual display server (Xvfb). See the [Troubleshooting](#troubleshooting) section for details.

### Usage

```bash
# Basic usage
img_deinterlace --input interlaced.jpg --output deinterlaced.jpg

# GPU acceleration
img_deinterlace -i interlaced.jpg -o deinterlaced.jpg --mode gpu

# Multi-threaded processing
img_deinterlace -i interlaced.jpg -o deinterlaced.jpg --mode threads

# SIMD-optimized processing
img_deinterlace -i interlaced.jpg -o deinterlaced.jpg --mode simd
```

## Example Result

Below is an example image showing the effect of deinterlacing. The image is wide, with the original (interlaced) version on the left and the deinterlaced result on the right:

![Deinterlacing Example](examples/example1.jpg)

## Processing Modes

| Mode | Description | Requirements |
|------|-------------|--------------|
| `default` | CPU-based blending | Any CPU |
| `async` | Asynchronous processing | Multi-core CPU |
| `threads` | Multi-threaded processing | Multi-core CPU |
| `gpu` | GPU acceleration | OpenGL 4.3+ |
| `simd` | SIMD-optimized processing | CPU with AVX2 support |

## Command Line Options

```
--input, -i     Input image file (required)
--output, -o    Output image file (default: output.jpeg)
--mode, -m      Processing mode: default, async, threads, gpu, simd
--help, -h      Show help message
```

## Supported Formats

- **Input**: JPEG, PNG, BMP, TIFF (via FFmpeg)
- **Output**: JPEG, PNG, BMP, TIFF (via FFmpeg)

## Architecture

The tool uses a pipeline architecture with three main components:

1. **Decoder** (`FFmpegDecNode`): Reads and decodes input images using FFmpeg
2. **Processor** (`Deinterlace*ProcNode`): Applies deinterlacing algorithms
3. **Encoder** (`FFmpegEncNode`): Encodes and writes output images using FFmpeg

### Dependencies

- **FFmpeg**: Built from source for maximum compatibility and control
- **GLFW**: Window management and OpenGL context creation
- **GLAD**: OpenGL loading library

## Algorithm

The deinterlacing algorithm blends odd and even scan lines:
- Even lines (0, 2, 4...) remain unchanged
- Odd lines (1, 3, 5...) are blended with the previous line: `new = (current + previous) / 2`

### SIMD Optimization

The SIMD mode uses AVX2 instructions to process 32 pixels simultaneously:
- Vectorized blending using `_mm256_avg_epu8` for optimal performance
- Automatic fallback to scalar processing for edge cases
- Compatible with all pixel formats supported by FFmpeg

## Development

### Project Structure
```
src/
├── main.cpp                 # Entry point
├── parser/                  # Command line parsing
├── nodes/                   # Pipeline components
│   ├── base/               # Base classes
│   ├── FFmpegDecNode       # Image decoder
│   ├── FFmpegEncNode       # Image encoder
│   └── Deinterlace*ProcNode # Processing nodes
```

### Adding New Processing Modes

1. Inherit from `Processor` class
2. Implement `updatePacket()` method
3. Add mode to `main.cpp` pipeline selection

## Troubleshooting

### GPU Mode Issues
- Ensure OpenGL 4.3+ is available
- Check GPU drivers are up to date
- Verify compute shader support

#### Docker GPU Mode
When running GPU mode in Docker containers, you need to start a virtual display server:

```bash
# Run container with GPU mode
docker run -it --rm img-deinterlace bash

# Inside container, start Xvfb and run the tool
export DISPLAY=:99
Xvfb :99 -screen 0 1024x768x24 &
./bin/Debug-linux-x86_64/img_deinterlace/img_deinterlace --input input.jpg --output output.jpg --mode gpu
```

### SIMD Mode Issues
- Ensure CPU supports AVX2 instructions
- Check compiler supports AVX2 intrinsics
- Verify `USE_SIMD` define is set during compilation

### Build Issues
- Ensure all dependencies are properly linked
- Check FFmpeg libraries are in the correct location
- Verify compiler supports C++17

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- FFmpeg for media processing capabilities
- GLFW for OpenGL context management
- GLAD for OpenGL loading
