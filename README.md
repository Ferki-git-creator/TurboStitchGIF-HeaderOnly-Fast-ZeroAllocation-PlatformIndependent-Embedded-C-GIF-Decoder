# GIF Decoder Library

![Header-only](https://img.shields.io/badge/header--only-brightgreen)
![Platform Independent](https://img.shields.io/badge/platform-independent-blue)
![Zero Allocations](https://img.shields.io/badge/zero%20allocations-success)
![MIT License](https://img.shields.io/badge/license-MIT-green)

**TurboStitchGIF** is a lightweight, header-only C library for decoding GIF images with a focus on efficiency and minimal resource usage. Designed for embedded systems and performance-critical applications, it provides a simple API for decoding both static and animated GIFs while maintaining a tiny footprint.

## ✨ Key Features

- **Single-header implementation** - Just include `gif.h` in your project
- **Zero dynamic allocations** - Works with user-provided memory buffers
- **Platform independent** - Pure C99 with no OS dependencies
- **Dual decoding modes**:
  - **Safe mode**: Minimal memory usage (default)
  - **Turbo mode**: Optimized for speed (define `GIF_MODE_TURBO`)
- **Low memory footprint** - Ideal for embedded systems and microcontrollers
- **Full animation support** - Handles frames, delays, transparency, and looping
- **Configurable limits** - Set canvas size and color limits via preprocessor defines

## 🚀 Getting Started

### Basic Usage

```c
#define GIF_IMPLEMENTATION
#include "gif.h"

// Define scratch buffer size (calculate using GIF_SCRATCH_BUFFER_REQUIRED_SIZE)
uint8_t scratch_buffer[SCRATCH_BUFFER_SIZE]; 

void process_gif(const uint8_t* gif_data, size_t gif_size) {
    GIF_Context ctx;
    if(gif_init(&ctx, gif_data, gif_size, 
                scratch_buffer, sizeof(scratch_buffer)) != GIF_SUCCESS) {
        // Handle error
        return;
    }

    int width, height;
    gif_get_info(&ctx, &width, &height);
    
    uint8_t* frame_buffer = malloc(width * height * 3);
    int delay_ms;
    
    while(gif_next_frame(&ctx, frame_buffer, &delay_ms) > 0) {
        // Process frame
        // delay_ms contains frame duration
    }
    
    gif_close(&ctx);
    free(frame_buffer);
}
```

### Configuration Options

Customize the library by defining these before including `gif.h`:

```c
#define GIF_MAX_WIDTH 800      // Max canvas width
#define GIF_MAX_HEIGHT 600     // Max canvas height
#define GIF_MAX_COLORS 256     // Max colors in palette
#define GIF_MODE_TURBO         // Enable faster decoding
#define GIF_MAX_CODE_SIZE 12   // LZW max code size (usually 12)
#include "gif.h"
```

## 📚 API Reference

### Core Functions

| Function | Description |
|----------|-------------|
| `gif_init()` | Initialize decoder context |
| `gif_get_info()` | Get GIF dimensions |
| `gif_next_frame()` | Decode next animation frame |
| `gif_rewind()` | Restart animation from beginning |
| `gif_close()` | Clean up decoder context |
| `gif_set_error_callback()` | Set custom error handler |

### Memory Requirements

The library requires a scratch buffer whose size depends on the selected mode:

```c
// Safe mode (default)
#define GIF_SCRATCH_BUFFER_REQUIRED_SIZE ...

// Turbo mode (faster)
#define GIF_SCRATCH_BUFFER_REQUIRED_SIZE ...
```

Calculate the exact size needed using these macros in your code.

## 🌟 Why Choose This Library?

- **Ultra-lightweight** - Perfect for resource-constrained environments
- **No external dependencies** - Just the C standard library
- **Simple integration** - Single header file simplifies project setup
- **Efficient decoding** - Optimized LZW implementation with Turbo mode
- **Robust error handling** - Detailed error codes and callback support
- **Transparency support** - Handles alpha channel properly
- **Animation features** - Full support for frame delays and looping

## 💡 Example Projects

Ideal for:
- Embedded systems displays
- IoT device interfaces
- Game development
- Resource-constrained applications
- Educational projects
- Custom image viewers

## ☕ Support

If you enjoy using this library and find it useful, consider supporting my work with a coffee!

<a href="https://ko-fi.com/ferki" target="_blank">
  <img src="https://ko-fi.com/img/githubbutton_sm.svg" alt="Buy Me a Coffee at ko-fi.com" height="36" />
</a>

Your support helps me continue maintaining and improving this project!

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
