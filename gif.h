/**
 * @file gif.h
 * @brief Header-only library for decoding GIF images.
 *
 * This library provides a minimalistic, header-only, and platform-independent API
 * for decoding animated GIF files. It combines LZW decoding optimizations
 * (Turbo LZW) with a compact design, avoiding dynamic memory allocations.
 *
 * @author Ferki
 * @date 16.07.2025
 * @version 0.3
 */

#ifndef GIF_H
#define GIF_H

#include <stdint.h>
#include <string.h> // For memcpy
#include <stddef.h> // For size_t

#ifdef __cplusplus
extern "C" {
#endif

// --- Constants and Configuration ---

/**
 * @brief Define GIF_IMPLEMENTATION in one C/C++ file to include the implementation.
 *
 * Example:
 * @code
 * // my_app.c
 * #define GIF_IMPLEMENTATION
 * #include "gif.h"
 * @endcode
 */
// #define GIF_IMPLEMENTATION

/**
 * @brief Define GIF_MODE_TURBO for faster decoding with a larger scratch buffer.
 *
 * If not defined, a more memory-efficient (but potentially slower) mode is used.
 */
// #define GIF_MODE_TURBO

/**
 * @brief Maximum supported GIF canvas width.
 * Can be overridden by defining GIF_MAX_WIDTH before including this header.
 */
#ifndef GIF_MAX_WIDTH
#define GIF_MAX_WIDTH 480
#endif

/**
 * @brief Maximum number of colors in a GIF palette.
 * Can be overridden by defining GIF_MAX_COLORS before including this header.
 */
#ifndef GIF_MAX_COLORS
#define GIF_MAX_COLORS 256
#endif

/**
 * @brief Maximum LZW code size.
 * Can be overridden by defining GIF_MAX_CODE_SIZE before including this header.
 */
#ifndef GIF_MAX_CODE_SIZE
#define GIF_MAX_CODE_SIZE 12
#endif

/** @brief Size of an LZW data block fragment. */
#define GIF_LZW_CHUNK_SIZE 255
/** @brief Base LZW buffer size for codes. */
#define GIF_LZW_BASE_BUF_SIZE (6 * GIF_LZW_CHUNK_SIZE)
/** @brief Number of entries in the LZW table. */
#define GIF_LZW_TABLE_ENTRIES (1 << GIF_MAX_CODE_SIZE) // 4096 entries

/**
 * @brief Defines the minimum required size for the internal scratch buffer.
 *
 * The user must provide a buffer of at least this size to gif_init().
 */
#ifdef GIF_MODE_TURBO
    /** @brief Size of the LZW dictionary symbols for Turbo mode. */
    #define GIF_SCRATCH_LZW_DICT_SYMBOLS_SIZE (GIF_LZW_TABLE_ENTRIES * sizeof(uint32_t))
    /** @brief Size of the LZW dictionary lengths for Turbo mode. */
    #define GIF_SCRATCH_LZW_DICT_LENGTHS_SIZE (GIF_LZW_TABLE_ENTRIES * sizeof(uint16_t))
    /** @brief Size of the LZW pixels suffix buffer for Turbo mode. */
    #define GIF_SCRATCH_LZW_PIXELS_SUFFIX_SIZE (GIF_LZW_TABLE_ENTRIES * sizeof(uint8_t))
    /** @brief Main LZW buffer size for Turbo mode. */
    #define GIF_SCRATCH_LZW_MAIN_BUF_SIZE (GIF_LZW_BASE_BUF_SIZE + (2 * GIF_LZW_TABLE_ENTRIES) + (GIF_LZW_TABLE_ENTRIES * 2) + GIF_MAX_WIDTH)
    /** @brief Total required scratch buffer size for Turbo mode. */
    #define GIF_SCRATCH_BUFFER_REQUIRED_SIZE (GIF_SCRATCH_LZW_MAIN_BUF_SIZE + GIF_SCRATCH_LZW_DICT_SYMBOLS_SIZE + GIF_SCRATCH_LZW_DICT_LENGTHS_SIZE + GIF_SCRATCH_LZW_PIXELS_SUFFIX_SIZE + GIF_MAX_WIDTH)
#else
    /** @brief Size of the LZW table for Safe mode. */
    #define GIF_SCRATCH_LZW_TABLE_SIZE (GIF_LZW_TABLE_ENTRIES * sizeof(uint16_t))
    /** @brief Size of the LZW pixels buffer for Safe mode. */
    #define GIF_SCRATCH_LZW_PIXELS_SIZE (GIF_LZW_TABLE_ENTRIES * 2 * sizeof(uint8_t))
    /** @brief Main LZW buffer size for Safe mode. */
    #define GIF_SCRATCH_LZW_MAIN_BUF_SIZE GIF_LZW_BASE_BUF_SIZE
    /** @brief Total required scratch buffer size for Safe mode. */
    #define GIF_SCRATCH_BUFFER_REQUIRED_SIZE (GIF_SCRATCH_LZW_MAIN_BUF_SIZE + GIF_SCRATCH_LZW_TABLE_SIZE + GIF_SCRATCH_LZW_PIXELS_SIZE + GIF_MAX_WIDTH)
#endif

// --- Error Codes ---
/**
 * @brief Enumeration of error codes that can be returned by the library.
 */
enum {
   /** @brief Operation completed successfully. */
   GIF_SUCCESS = 0,
   /** @brief Error during LZW data decoding. */
   GIF_ERROR_DECODE,
   /** @brief Invalid parameter passed (e.g., NULL pointer). */
   GIF_ERROR_INVALID_PARAM,
   /** @brief The GIF file has an invalid format or is corrupted. */
   GIF_ERROR_BAD_FILE,
   /** @brief Unexpected end of file encountered while reading data. */
   GIF_ERROR_EARLY_EOF,
   /** @brief No next frame found to decode. */
   GIF_ERROR_NO_FRAME,
   /** @brief The user-provided buffer is too small. */
   GIF_ERROR_BUFFER_TOO_SMALL,
   /** @brief Invalid frame dimensions (e.g., zero width/height, or extends beyond canvas). */
   GIF_ERROR_INVALID_FRAME_DIMENSIONS,
   /** @brief Unsupported color depth (e.g., palette size exceeds GIF_MAX_COLORS). */
   GIF_ERROR_UNSUPPORTED_COLOR_DEPTH
};

/**
 * @brief Type definition for an error callback function.
 *
 * The user can provide their own function for logging or handling errors.
 * @param error_code The error code from the enumeration.
 * @param message A descriptive message about the error.
 */
typedef void (*GIF_ErrorCallback)(int error_code, const char* message);

// --- Context Structure ---
/**
 * @brief Structure holding the state of the GIF decoder.
 *
 * All internal data and buffers are managed through this structure.
 */
typedef struct {
    /** @brief Pointer to the raw GIF data provided by the user. */
    const uint8_t *gif_data;
    /** @brief Total size of the GIF data. */
    size_t gif_size;
    /** @brief Current read position within `gif_data`. */
    size_t current_pos;

    /** @brief Width of the GIF canvas. */
    uint32_t canvas_width;
    /** @brief Height of the GIF canvas. */
    uint32_t canvas_height;
    /** @brief Width of the current frame. */
    uint32_t frame_width;
    /** @brief Height of the current frame. */
    uint32_t frame_height;
    /** @brief X-offset of the current frame relative to the canvas. */
    uint16_t frame_x_off;
    /** @brief Y-offset of the current frame relative to the canvas. */
    uint16_t frame_y_off;
    /** @brief Delay for the current frame in milliseconds. */
    uint16_t frame_delay_ms;
    /**
     * @brief Animation loop count.
     * -1 for infinite loop, 0 for single play, >0 for a specific count.
     */
    int16_t loop_count;
    /** @brief Index of the background color. */
    uint8_t background_index;
    /** @brief Index of the transparent color. */
    uint8_t transparent_index;
    /** @brief Flag indicating if the current frame has transparency. */
    uint8_t has_transparency;
    /**
     * @brief Disposal method for the frame.
     * 0: No disposal, 1: Do not dispose, 2: Restore to background, 3: Restore to previous.
     */
    uint8_t disposal_method;
    /** @brief Packed field from the image descriptor (contains interlacing flag). */
    uint8_t ucGIFBits;

    /** @brief Global color palette (RGB888 format). */
    uint8_t global_palette_colors[GIF_MAX_COLORS * 3];
    /** @brief Local color palette (RGB888 format). */
    uint8_t local_palette_colors[GIF_MAX_COLORS * 3];
    /** @brief Pointer to the active palette (either global or local). */
    uint8_t *active_palette_colors;

    /** @brief Initial LZW code size for the current frame. */
    uint8_t lzw_code_start_size;
    /** @brief Flag indicating if the current frame's LZW data is exhausted. */
    uint8_t lzw_end_of_frame;
    /** @brief Current read offset within `scratch_lzw_buffer`. */
    int lzw_read_offset;
    /** @brief Total size of LZW data currently in `scratch_lzw_buffer`. */
    int lzw_data_size;

    /** @brief Pointer to the LZW buffer within the user-provided scratch buffer. */
    uint8_t *scratch_lzw_buffer;
#ifdef GIF_MODE_TURBO
    /** @brief Pointer to the LZW dictionary symbols for Turbo mode. */
    uint8_t *scratch_lzw_dict_symbols;
    /** @brief Pointer to the LZW dictionary lengths for Turbo mode. */
    uint8_t *scratch_lzw_dict_lengths;
    /** @brief Pointer to the LZW pixels suffix buffer for Turbo mode. */
    uint8_t *scratch_lzw_pixels_suffix;
#else
    /** @brief Pointer to the LZW table for Safe mode. */
    uint16_t *scratch_lzw_table;
    /** @brief Pointer to the LZW pixels buffer for Safe mode. */
    uint8_t *scratch_lzw_pixels;
#endif
    /** @brief Pointer to the buffer for reconstructing pixel lines. */
    uint8_t *scratch_line_buffer;

    /** @brief Position in `gif_data` where animation frames start. */
    size_t anim_start_pos;
    /** @brief Small internal buffer for reading headers/extensions. */
    uint8_t file_buf[GIF_LZW_CHUNK_SIZE + 32];

    /** @brief Callback function for error handling. */
    GIF_ErrorCallback error_callback;
} GIF_Context;

// --- API Functions ---

/**
 * @brief Initializes the GIF decoder context.
 *
 * This function prepares the GIF_Context structure for decoding a GIF file.
 * It reads the GIF header and sets up the initial state.
 *
 * @param ctx Pointer to the GIF_Context structure to initialize.
 * @param data Pointer to the raw GIF data in memory.
 * @param size Size of the raw GIF data.
 * @param scratch_buffer Pointer to a user-provided scratch buffer for internal operations.
 * Its size must be at least GIF_SCRATCH_BUFFER_REQUIRED_SIZE.
 * @param scratch_buffer_size Size of the provided scratch buffer.
 * @return GIF_SUCCESS on success, or an error code.
 */
int gif_init(GIF_Context *ctx, const uint8_t *data, size_t size, uint8_t *scratch_buffer, size_t scratch_buffer_size);

/**
 * @brief Gets the width and height of the GIF canvas.
 *
 * This function returns the dimensions of the GIF logical screen.
 *
 * @param ctx Pointer to the initialized GIF_Context structure.
 * @param width Pointer to an integer where the canvas width will be stored.
 * @param height Pointer to an integer where the canvas height will be stored.
 * @return GIF_SUCCESS on success, or an error code.
 */
int gif_get_info(GIF_Context *ctx, int *width, int *height);

/**
 * @brief Decodes and renders the next frame of the GIF.
 *
 * This function decodes the LZW data of the current frame and renders it
 * into the provided buffer in RGB888 format (3 bytes per pixel).
 *
 * @param ctx Pointer to the initialized GIF_Context structure.
 * @param frame_buffer Pointer to a user-provided buffer where the decoded frame
 * will be rendered. This buffer should be large enough to hold
 * `width * height * 3` bytes.
 * @param delay_ms Pointer to an integer where the delay for the current frame
 * in milliseconds will be stored.
 * @return 1 if a frame was successfully decoded; 0 if the animation has finished;
 * or -1 on a decoding error.
 */
int gif_next_frame(GIF_Context *ctx, uint8_t *frame_buffer, int *delay_ms);

/**
 * @brief Rewinds the GIF animation to the beginning.
 *
 * This function resets the internal state of the decoder, allowing the GIF
 * to be played again from the first frame.
 *
 * @param ctx Pointer to the initialized GIF_Context structure.
 */
void gif_rewind(GIF_Context *ctx);

/**
 * @brief Cleans up the GIF context.
 *
 * This function clears all internal fields of the GIF_Context structure.
 *
 * @param ctx Pointer to the GIF_Context structure to clean up.
 */
void gif_close(GIF_Context *ctx);

/**
 * @brief Sets the error callback function.
 *
 * If set, this function will be called when internal errors occur.
 * By default, errors are not logged.
 *
 * @param ctx Pointer to the initialized GIF_Context structure.
 * @param callback The callback function, or NULL to disable error logging.
 */
void gif_set_error_callback(GIF_Context *ctx, GIF_ErrorCallback callback);


#ifdef __cplusplus
}
#endif

// --- Implementation (only if GIF_IMPLEMENTATION is defined) ---
#ifdef GIF_IMPLEMENTATION

/**
 * @brief Reports an error by calling the error callback if set.
 * @param ctx Pointer to the GIF context.
 * @param error_code The error code.
 * @param message The error message.
 */
static void gif_report_error(GIF_Context *ctx, int error_code, const char* message) {
    if (ctx && ctx->error_callback) {
        ctx->error_callback(error_code, message);
    }
}

/**
 * @brief Reads a 16-bit unsigned integer in little-endian byte order.
 * @param data Pointer to the data.
 * @return The 16-bit unsigned integer.
 */
static inline uint16_t gif_read_u16_le(const uint8_t *data) {
    uint16_t value;
    memcpy(&value, data, 2);
    return value;
}

/**
 * @brief Reads bytes from the GIF data buffer into a destination buffer.
 * @param ctx Pointer to the GIF context.
 * @param buffer Pointer to the destination buffer.
 * @param len Number of bytes to read.
 * @return Number of bytes actually read.
 */
static size_t gif_read_bytes_internal(GIF_Context *ctx, uint8_t *buffer, size_t len) {
    size_t bytes_to_read = len;
    if (ctx->current_pos + bytes_to_read > ctx->gif_size) {
        bytes_to_read = ctx->gif_size - ctx->current_pos;
    }
    if (bytes_to_read > 0) {
        memcpy(buffer, ctx->gif_data + ctx->current_pos, bytes_to_read);
        ctx->current_pos += bytes_to_read;
    }
    return bytes_to_read;
}

/**
 * @brief Skips bytes in the GIF data buffer.
 * @param ctx Pointer to the GIF context.
 * @param len Number of bytes to skip.
 */
static void gif_skip_bytes_internal(GIF_Context *ctx, size_t len) {
    if (ctx->current_pos + len <= ctx->gif_size) {
        ctx->current_pos += len;
    } else {
        ctx->current_pos = ctx->gif_size;
    }
}

/**
 * @brief Reads a single byte from the GIF data buffer.
 * @param ctx Pointer to the GIF context.
 * @return The byte read, or 0 if EOF is reached or an error occurs.
 */
static uint8_t gif_read_byte_internal(GIF_Context *ctx) {
    if (ctx->current_pos < ctx->gif_size) {
        return ctx->gif_data[ctx->current_pos++];
    }
    gif_report_error(ctx, GIF_ERROR_EARLY_EOF, "Unexpected EOF while reading byte.");
    return 0;
}

/**
 * @brief Discards sub-blocks (used for extensions).
 * @param ctx Pointer to the GIF context.
 */
static void gif_discard_sub_blocks(GIF_Context *ctx) {
    uint8_t size;
    do {
        size = gif_read_byte_internal(ctx);
        if (size == 0 && ctx->current_pos >= ctx->gif_size) {
            gif_report_error(ctx, GIF_ERROR_EARLY_EOF, "Unexpected EOF while discarding sub-blocks.");
            return;
        }
        gif_skip_bytes_internal(ctx, size);
    } while (size);
}

/**
 * @brief Reads a Graphic Control Extension block.
 * @param ctx Pointer to the GIF context.
 */
static void gif_read_graphic_control_ext(GIF_Context *ctx) {
    uint8_t rdit;
    gif_skip_bytes_internal(ctx, 1); // Block size (always 4)
    rdit = gif_read_byte_internal(ctx);
    ctx->disposal_method = (rdit >> 2) & 3;
    ctx->has_transparency = rdit & 1;
    ctx->frame_delay_ms = gif_read_u16_le(ctx->gif_data + ctx->current_pos) * 10;
    gif_skip_bytes_internal(ctx, 2); // Delay time
    ctx->transparent_index = gif_read_byte_internal(ctx);
    gif_skip_bytes_internal(ctx, 1); // Block terminator
}

/**
 * @brief Reads an Application Extension block (e.g., Netscape loop count).
 * @param ctx Pointer to the GIF context.
 */
static void gif_read_application_ext(GIF_Context *ctx) {
    uint8_t block_size = gif_read_byte_internal(ctx); // Should be 11
    if (block_size == 11) {
        uint8_t app_id[8];
        uint8_t app_auth_code[3];
        gif_read_bytes_internal(ctx, app_id, 8);
        gif_read_bytes_internal(ctx, app_auth_code, 3);

        uint8_t sub_block_size = gif_read_byte_internal(ctx); // Should be 3
        if (sub_block_size != 3) { // Check for expected sub-block size
            gif_report_error(ctx, GIF_ERROR_BAD_FILE, "Unexpected sub-block size in Application Extension.");
            gif_discard_sub_blocks(ctx); // Discard remaining sub-blocks for this extension
            return;
        }
        gif_skip_bytes_internal(ctx, 1); // Sub-block ID (always 1)
        ctx->loop_count = gif_read_u16_le(ctx->gif_data + ctx->current_pos);
        gif_skip_bytes_internal(ctx, 2); // Loop count
    }
    gif_discard_sub_blocks(ctx); // Discard remaining sub-blocks for this extension
}

/**
 * @brief Reads an extension block.
 * @param ctx Pointer to the GIF context.
 */
static void gif_read_ext(GIF_Context *ctx) {
    uint8_t label = gif_read_byte_internal(ctx);
    switch (label) {
        case 0xF9: // Graphic Control Extension
            gif_read_graphic_control_ext(ctx);
            break;
        case 0xFF: // Application Extension
            gif_read_application_ext(ctx);
            break;
        case 0x01: // Plain Text Extension (discarded)
        case 0xFE: // Comment Extension (discarded)
        default:
            gif_report_error(ctx, GIF_ERROR_DECODE, "Unknown GIF extension encountered.");
            gif_discard_sub_blocks(ctx);
            break;
    }
}

/**
 * @brief Fills the LZW buffer with more data if needed.
 * @param ctx Pointer to the GIF context.
 * @return 1 if data was successfully read or LZW stream end is reached; 0 on error.
 */
static int gif_get_more_lzw_data(GIF_Context *ctx) {
    int bytes_in_buffer = ctx->lzw_data_size - ctx->lzw_read_offset;
    int lzw_buf_capacity = GIF_LZW_BASE_BUF_SIZE;
    uint8_t c;

    if (ctx->lzw_end_of_frame) {
        return 1; // End of frame data already reached
    }

    // Shift remaining data to the beginning of the buffer
    if (ctx->lzw_read_offset > 0) {
        if (bytes_in_buffer > 0) {
            memmove(ctx->scratch_lzw_buffer, ctx->scratch_lzw_buffer + ctx->lzw_read_offset, bytes_in_buffer);
        }
        ctx->lzw_data_size = bytes_in_buffer;
        ctx->lzw_read_offset = 0;
    }

    // Read more blocks until buffer is full or end of frame
    while (ctx->lzw_data_size < (lzw_buf_capacity - GIF_LZW_CHUNK_SIZE) && ctx->current_pos < ctx->gif_size) {
        c = gif_read_byte_internal(ctx);
        if (c == 0) { // Block terminator
            ctx->lzw_end_of_frame = 1;
            break;
        }
        size_t bytes_read = gif_read_bytes_internal(ctx, ctx->scratch_lzw_buffer + ctx->lzw_data_size, c);
        ctx->lzw_data_size += bytes_read;
        if (bytes_read < c) { // Early EOF
            gif_report_error(ctx, GIF_ERROR_EARLY_EOF, "Early EOF while reading LZW data block.");
            return 0;
        }
    }
    return (c != 0 || ctx->lzw_end_of_frame);
}

/**
 * @brief LZW decoding helper function for copying bytes (optimized).
 * @param buf Buffer containing LZW data.
 * @param offset Write offset.
 * @param symbols Pointer to the LZW dictionary symbols array.
 * @param lengths Pointer to the LZW dictionary lengths array.
 * @return Length of the copied data. Returns 0 on buffer overflow.
 */
static int gif_lzw_copy_bytes(uint8_t *buf, int offset, uint32_t *symbols, uint16_t *lengths) {
    int len = *lengths;
    uint32_t u32Offset = *symbols;
    uint8_t *s = &buf[u32Offset & 0x7fffff];
    uint8_t *d = &buf[offset];
    uint8_t *pEnd = &d[len];

    // Check for potential buffer overflow before copying
    if (offset + len > GIF_LZW_BASE_BUF_SIZE + GIF_LZW_TABLE_ENTRIES * 2) { // Max possible size for scratch_lzw_buffer
        // This indicates a severe decoding issue or corrupted GIF data
        return 0; // Indicate failure
    }

    // Optimized copy using larger types if available and aligned
    while (d + sizeof(uint32_t) <= pEnd && (uintptr_t)s % sizeof(uint32_t) == 0 && (uintptr_t)d % sizeof(uint32_t) == 0) {
        *(uint32_t *)d = *(uint32_t *)s;
        s += sizeof(uint32_t);
        d += sizeof(uint32_t);
    }
    while (d < pEnd) {
        *d++ = *s++;
    }

    if (u32Offset & 0x800000) { // Check for extended suffix byte
        d = pEnd;
        len++;
        // Check for potential buffer overflow for the added byte
        if (offset + len > GIF_LZW_BASE_BUF_SIZE + GIF_LZW_TABLE_ENTRIES * 2) {
            return 0; // Indicate failure
        }
        *symbols = offset;
        *d = (uint8_t)(u32Offset >> 24);
        *lengths = (uint16_t)len;
    }
    return len;
}

/**
 * @brief Macro for getting the next LZW code from the buffer.
 * @param ctx Pointer to the GIF context.
 * @param p Pointer to the current position in the LZW buffer.
 * @param bitnum Current bit offset.
 * @param codesize Current code size.
 * @param sMask Mask for extracting the code.
 * @param code Variable to store the retrieved code.
 * @param ulBits Variable to store the read bits.
 */
#define GET_LZW_CODE(ctx, p, bitnum, codesize, sMask, code, ulBits) \
    do { \
        if (bitnum > (32 - codesize)) { /* Assuming 32-bit ulBits */ \
            ctx->lzw_read_offset += (bitnum >> 3); \
            bitnum &= 7; \
            if (!gif_get_more_lzw_data(ctx)) { /* Ensure more data is available */ \
                code = eoi_code; /* Indicate end of input if no more data */ \
                break; \
            } \
            p = ctx->scratch_lzw_buffer + ctx->lzw_read_offset; \
            memcpy(&ulBits, p, sizeof(uint32_t)); /* Read 4 bytes for ulBits */ \
        } \
        code = (uint16_t)((ulBits >> bitnum) & sMask); \
        bitnum += codesize; \
    } while(0)

/**
 * @brief Decodes LZW data for a single frame.
 * @param ctx Pointer to the GIF context.
 * @param frame_buffer Pointer to the buffer where the frame will be rendered.
 * @return GIF_SUCCESS on success, or an error code.
 */
static int gif_decode_lzw(GIF_Context *restrict ctx, uint8_t *restrict frame_buffer) {
    int i, bitnum;
    uint16_t code, oldcode, codesize, nextcode, nextlim;
    uint16_t clear_code, eoi_code;
    uint32_t sMask;
    uint8_t c = 0; // Initialize 'c'
    uint8_t *p;
    uint32_t ulBits;

    uint32_t *lzw_symbols = (uint32_t*)ctx->scratch_lzw_dict_symbols;
    uint16_t *lzw_lengths = (uint16_t*)ctx->scratch_lzw_dict_lengths;
    uint8_t *lzw_pixels_suffix = (uint8_t*)ctx->scratch_lzw_pixels_suffix;

    uint16_t *lzw_table = (uint16_t*)ctx->scratch_lzw_table;
    uint8_t *lzw_pixels = (uint8_t*)ctx->scratch_lzw_pixels;

    int current_pixel_idx = 0;
    int current_line_idx = 0;
    int interlaced_pass = 0;
    int interlaced_line_offset[] = {0, 4, 2, 1};
    int interlaced_line_stride[] = {8, 8, 4, 2};

    ctx->lzw_read_offset = 0;
    ctx->lzw_data_size = 0;
    ctx->lzw_end_of_frame = 0;
    if (!gif_get_more_lzw_data(ctx)) {
        gif_report_error(ctx, GIF_ERROR_EARLY_EOF, "Failed to get initial LZW data for frame.");
        return GIF_ERROR_EARLY_EOF;
    }

    p = ctx->scratch_lzw_buffer;
    memcpy(&ulBits, p, sizeof(uint32_t));

    clear_code = 1 << ctx->lzw_code_start_size;
    eoi_code = clear_code + 1;

#ifdef GIF_MODE_TURBO
    for (i = 0; i < clear_code; i++) {
        lzw_symbols[i] = (uint32_t)(ctx->scratch_lzw_pixels_suffix - ctx->scratch_lzw_buffer) + i;
        lzw_lengths[i] = 1;
        lzw_pixels_suffix[i] = (uint8_t)i;
    }
init_codetable_turbo:
    codesize = ctx->lzw_code_start_size + 1;
    sMask = (1 << codesize) - 1;
    nextcode = eoi_code + 1;
    nextlim = (1 << codesize);
    bitnum = 0;

    GET_LZW_CODE(ctx, p, bitnum, codesize, sMask, code, ulBits);
    if (code == clear_code) {
        GET_LZW_CODE(ctx, p, bitnum, codesize, sMask, code, ulBits);
    }
    if (code >= GIF_LZW_TABLE_ENTRIES || lzw_lengths[code] == 0) {
        gif_report_error(ctx, GIF_ERROR_DECODE, "Invalid initial LZW code in Turbo mode.");
        return GIF_ERROR_DECODE;
    }
    memcpy(ctx->scratch_line_buffer + current_pixel_idx, ctx->scratch_lzw_buffer + (lzw_symbols[code] & 0x7fffff), lzw_lengths[code]);
    current_pixel_idx += lzw_lengths[code];

    oldcode = code;
    GET_LZW_CODE(ctx, p, bitnum, codesize, sMask, code, ulBits);

    while (code != eoi_code) {
        if (code == clear_code) {
            goto init_codetable_turbo;
        }
        if (code != eoi_code) {
            int len_copied;
            if (nextcode < GIF_LZW_TABLE_ENTRIES) {
                if (code != nextcode) {
                    len_copied = gif_lzw_copy_bytes(ctx->scratch_lzw_buffer, current_pixel_idx, &lzw_symbols[code], &lzw_lengths[code]);
                    if (len_copied == 0) { gif_report_error(ctx, GIF_ERROR_DECODE, "LZW copy failed (buffer overflow)."); return GIF_ERROR_DECODE; }
                    lzw_symbols[nextcode] = (lzw_symbols[oldcode] | 0x800000 | (ctx->scratch_lzw_buffer[current_pixel_idx] << 24));
                    lzw_lengths[nextcode] = lzw_lengths[oldcode];
                    current_pixel_idx += len_copied;
                } else {
                    len_copied = gif_lzw_copy_bytes(ctx->scratch_lzw_buffer, current_pixel_idx, &lzw_symbols[oldcode], &lzw_lengths[oldcode]);
                    if (len_copied == 0) { gif_report_error(ctx, GIF_ERROR_DECODE, "LZW copy failed (buffer overflow)."); return GIF_ERROR_DECODE; }
                    lzw_lengths[nextcode] = (uint16_t)(len_copied + 1);
                    lzw_symbols[nextcode] = (uint32_t)current_pixel_idx;
                    c = ctx->scratch_lzw_buffer[current_pixel_idx];
                    current_pixel_idx += len_copied;
                    ctx->scratch_lzw_buffer[current_pixel_idx++] = c;
                }
            } else {
                 if (code >= GIF_LZW_TABLE_ENTRIES || lzw_lengths[code] == 0) {
                     gif_report_error(ctx, GIF_ERROR_DECODE, "LZW dictionary full, but invalid code encountered in Turbo mode.");
                     return GIF_ERROR_DECODE;
                 }
                len_copied = gif_lzw_copy_bytes(ctx->scratch_lzw_buffer, current_pixel_idx, &lzw_symbols[code], &lzw_lengths[code]);
                if (len_copied == 0) { gif_report_error(ctx, GIF_ERROR_DECODE, "LZW copy failed (buffer overflow)."); return GIF_ERROR_DECODE; }
                current_pixel_idx += len_copied;
            }
            nextcode++;
            if (nextcode >= nextlim && codesize < GIF_MAX_CODE_SIZE) {
                codesize++;
                nextlim <<= 1;
                sMask = (sMask << 1) | 1;
            }

            if (current_pixel_idx >= ctx->frame_width) {
                int y_draw = current_line_idx;
                if (ctx->ucGIFBits & 0x40) {
                    y_draw = interlaced_line_offset[interlaced_pass] + current_line_idx * interlaced_line_stride[interlaced_pass];
                    while (y_draw >= ctx->frame_height && interlaced_pass < 3) {
                        interlaced_pass++;
                        current_line_idx = 0;
                        y_draw = interlaced_line_offset[interlaced_pass] + current_line_idx * interlaced_line_stride[interlaced_pass];
                    }
                    if (y_draw >= ctx->frame_height) {
                        gif_report_error(ctx, GIF_ERROR_DECODE, "Interlaced GIF decoding error: line out of bounds.");
                        return GIF_ERROR_DECODE;
                    }
                }

                uint8_t *dest_row_start = frame_buffer + (ctx->frame_y_off + y_draw) * ctx->canvas_width * 3 + ctx->frame_x_off * 3;
                uint8_t *src_pixels = ctx->scratch_line_buffer;
                uint8_t *palette = ctx->active_palette_colors; // Cache palette pointer

                for (i = 0; i < ctx->frame_width; i++) {
                    uint8_t pixel_index = src_pixels[i];
                    if (ctx->has_transparency && pixel_index == ctx->transparent_index) {
                        if (ctx->disposal_method == 2) {
                            memcpy(dest_row_start + i*3, palette + ctx->background_index*3, 3);
                        }
                    } else {
                        memcpy(dest_row_start + i*3, palette + pixel_index*3, 3);
                    }
                }
                current_pixel_idx = 0;
                current_line_idx++;
            }
            oldcode = code;
            GET_LZW_CODE(ctx, p, bitnum, codesize, sMask, code, ulBits);
        }
    }
#else // GIF_MODE_SAFE
    for (i = 0; i < clear_code; i++) {
        lzw_pixels[i] = lzw_pixels[GIF_LZW_TABLE_ENTRIES + i] = (uint8_t)i;
        lzw_table[i] = 0xFFFF; // LINK_END equivalent
    }
init_codetable_safe:
    codesize = ctx->lzw_code_start_size + 1;
    sMask = (1 << codesize) - 1;
    nextcode = eoi_code + 1;
    nextlim = (1 << codesize);
    bitnum = 0;
    memset(&lzw_table[clear_code], 0xFF, sizeof(uint16_t) * (GIF_LZW_TABLE_ENTRIES - clear_code)); // LINK_UNUSED equivalent

    GET_LZW_CODE(ctx, p, bitnum, codesize, sMask, code, ulBits);
    if (code == clear_code) {
        GET_LZW_CODE(ctx, p, bitnum, codesize, sMask, code, ulBits);
    }
    if (code >= GIF_LZW_TABLE_ENTRIES) {
        gif_report_error(ctx, GIF_ERROR_DECODE, "Invalid initial LZW code in Safe mode.");
        return GIF_ERROR_DECODE;
    }
    c = oldcode = code;
    
    uint8_t temp_line_buf[GIF_MAX_WIDTH];
    int temp_idx = GIF_MAX_WIDTH;
    uint16_t current_lzw_code = code;
    while(current_lzw_code != 0xFFFF && temp_idx > 0) {
        temp_line_buf[--temp_idx] = lzw_pixels[current_lzw_code];
        current_lzw_code = lzw_table[current_lzw_code];
    }
    memcpy(ctx->scratch_line_buffer, temp_line_buf + temp_idx, (size_t)(GIF_MAX_WIDTH - temp_idx));
    current_pixel_idx += (GIF_MAX_WIDTH - temp_idx);


    while (code != eoi_code) {
        GET_LZW_CODE(ctx, p, bitnum, codesize, sMask, code, ulBits);
        if (code == clear_code) {
            goto init_codetable_safe;
        }
        if (code != eoi_code) {
            temp_idx = GIF_MAX_WIDTH;
            current_lzw_code = code;
            while(current_lzw_code != 0xFFFF && temp_idx > 0) {
                temp_line_buf[--temp_idx] = lzw_pixels[current_lzw_code];
                current_lzw_code = lzw_table[current_lzw_code];
            }
            int pixels_to_copy = GIF_MAX_WIDTH - temp_idx;
            memcpy(ctx->scratch_line_buffer + current_pixel_idx, temp_line_buf + temp_idx, (size_t)pixels_to_copy);
            current_pixel_idx += pixels_to_copy;

            if (nextcode < GIF_LZW_TABLE_ENTRIES) {
                lzw_table[nextcode] = oldcode;
                lzw_pixels[nextcode] = c;
                lzw_pixels[GIF_LZW_TABLE_ENTRIES + nextcode] = c = lzw_pixels[code];
            }
            nextcode++;
            if (nextcode >= nextlim && codesize < GIF_MAX_CODE_SIZE) {
                codesize++;
                nextlim <<= 1;
                sMask = nextlim - 1;
            }

            if (current_pixel_idx >= ctx->frame_width) {
                int y_draw = current_line_idx;
                if (ctx->ucGIFBits & 0x40) {
                    y_draw = interlaced_line_offset[interlaced_pass] + current_line_idx * interlaced_line_stride[interlaced_pass];
                    while (y_draw >= ctx->frame_height && interlaced_pass < 3) {
                        interlaced_pass++;
                        current_line_idx = 0;
                        y_draw = interlaced_line_offset[interlaced_pass] + current_line_idx * interlaced_line_stride[interlaced_pass];
                    }
                    if (y_draw >= ctx->frame_height) {
                        gif_report_error(ctx, GIF_ERROR_DECODE, "Interlaced GIF decoding error: line out of bounds.");
                        return GIF_ERROR_DECODE;
                    }
                }

                uint8_t *dest_row_start = frame_buffer + (ctx->frame_y_off + y_draw) * ctx->canvas_width * 3 + ctx->frame_x_off * 3;
                uint8_t *src_pixels = ctx->scratch_line_buffer;
                uint8_t *palette = ctx->active_palette_colors; // Cache palette pointer

                for (i = 0; i < ctx->frame_width; i++) {
                    uint8_t pixel_index = src_pixels[i];
                    if (ctx->has_transparency && pixel_index == ctx->transparent_index) {
                        if (ctx->disposal_method == 2) {
                            memcpy(dest_row_start + i*3, palette + ctx->background_index*3, 3);
                        }
                    } else {
                        memcpy(dest_row_start + i*3, palette + pixel_index*3, 3);
                    }
                }
                current_pixel_idx = 0;
                current_line_idx++;
            }
            oldcode = code;
        }
    }
#endif // GIF_MODE_TURBO

    return GIF_SUCCESS;
}

// --- API Function Implementations ---

int gif_init(GIF_Context *ctx, const uint8_t *data, size_t size, uint8_t *scratch_buffer, size_t scratch_buffer_size) {
    if (!ctx || !data || size == 0 || !scratch_buffer) {
        gif_report_error(ctx, GIF_ERROR_INVALID_PARAM, "Invalid parameters for gif_init.");
        return GIF_ERROR_INVALID_PARAM;
    }

    if (scratch_buffer_size < GIF_SCRATCH_BUFFER_REQUIRED_SIZE) {
        gif_report_error(ctx, GIF_ERROR_BUFFER_TOO_SMALL,
            "Scratch buffer too small. Required: %zu bytes.",
            GIF_SCRATCH_BUFFER_REQUIRED_SIZE);
        return GIF_ERROR_BUFFER_TOO_SMALL;
    }

    memset(ctx, 0, sizeof(GIF_Context));
    ctx->gif_data = data;
    ctx->gif_size = size;
    ctx->current_pos = 0;
    ctx->loop_count = -1; // Default to infinite loop if not specified

    uint8_t *current_scratch_ptr = scratch_buffer;
#ifdef GIF_MODE_TURBO
    ctx->scratch_lzw_buffer = current_scratch_ptr;
    current_scratch_ptr += GIF_SCRATCH_LZW_MAIN_BUF_SIZE;
    ctx->scratch_lzw_dict_symbols = current_scratch_ptr;
    current_scratch_ptr += GIF_SCRATCH_LZW_DICT_SYMBOLS_SIZE;
    ctx->scratch_lzw_dict_lengths = current_scratch_ptr;
    current_scratch_ptr += GIF_SCRATCH_LZW_DICT_LENGTHS_SIZE;
    ctx->scratch_lzw_pixels_suffix = current_scratch_ptr;
    current_scratch_ptr += GIF_SCRATCH_LZW_PIXELS_SUFFIX_SIZE;
#else
    ctx->scratch_lzw_buffer = current_scratch_ptr;
    current_scratch_ptr += GIF_SCRATCH_LZW_MAIN_BUF_SIZE;
    ctx->scratch_lzw_table = (uint16_t*)current_scratch_ptr;
    current_scratch_ptr += GIF_SCRATCH_LZW_TABLE_SIZE;
    ctx->scratch_lzw_pixels = current_scratch_ptr;
    current_scratch_ptr += GIF_SCRATCH_LZW_PIXELS_SIZE;
#endif
    ctx->scratch_line_buffer = current_scratch_ptr;

    if (gif_read_bytes_internal(ctx, ctx->file_buf, 13) < 13) {
        gif_report_error(ctx, GIF_ERROR_EARLY_EOF, "Early EOF while reading GIF header.");
        return GIF_ERROR_EARLY_EOF;
    }

    if (memcmp(ctx->file_buf, "GIF", 3) != 0 || (memcmp(ctx->file_buf + 3, "87a", 3) != 0 && memcmp(ctx->file_buf + 3, "89a", 3) != 0)) {
        gif_report_error(ctx, GIF_ERROR_BAD_FILE, "Invalid GIF signature.");
        return GIF_ERROR_BAD_FILE;
    }

    ctx->canvas_width = gif_read_u16_le(ctx->file_buf + 6);
    ctx->canvas_height = gif_read_u16_le(ctx->file_buf + 8);

    uint8_t fdsz = ctx->file_buf[10];
    if (fdsz & 0x80) { // Global Color Table Flag
        int gct_size = 1 << ((fdsz & 0x07) + 1);
        if (gct_size > GIF_MAX_COLORS) {
            gif_report_error(ctx, GIF_ERROR_UNSUPPORTED_COLOR_DEPTH, "Global Color Table size exceeds GIF_MAX_COLORS.");
            return GIF_ERROR_UNSUPPORTED_COLOR_DEPTH;
        }
        if (gif_read_bytes_internal(ctx, ctx->global_palette_colors, gct_size * 3) < gct_size * 3) {
            gif_report_error(ctx, GIF_ERROR_EARLY_EOF, "Early EOF while reading Global Color Table.");
            return GIF_ERROR_EARLY_EOF;
        }
    }
    ctx->background_index = ctx->file_buf[11];
    ctx->active_palette_colors = ctx->global_palette_colors;

    ctx->anim_start_pos = ctx->current_pos;
    return GIF_SUCCESS;
}

int gif_get_info(GIF_Context *ctx, int *width, int *height) {
    if (!ctx || !width || !height) {
        gif_report_error(ctx, GIF_ERROR_INVALID_PARAM, "Invalid parameters for gif_get_info.");
        return GIF_ERROR_INVALID_PARAM;
    }
    *width = (int)ctx->canvas_width;
    *height = (int)ctx->canvas_height;
    return GIF_SUCCESS;
}

int gif_next_frame(GIF_Context *ctx, uint8_t *frame_buffer, int *delay_ms) {
    if (!ctx || !frame_buffer || !delay_ms) {
        gif_report_error(ctx, GIF_ERROR_INVALID_PARAM, "Invalid parameters for gif_next_frame.");
        return GIF_ERROR_INVALID_PARAM;
    }

    if (ctx->current_pos >= ctx->gif_size) {
        if (ctx->loop_count == -1 || ctx->loop_count > 0) {
            if (ctx->loop_count > 0) ctx->loop_count--;
            gif_rewind(ctx);
        } else {
            return 0; // Animation finished
        }
    }

    uint8_t separator;
    while (ctx->current_pos < ctx->gif_size) {
        separator = gif_read_byte_internal(ctx);
        if (separator == 0x3B) { // GIF Trailer
            if (ctx->loop_count == -1 || ctx->loop_count > 0) {
                if (ctx->loop_count > 0) ctx->loop_count--;
                gif_rewind(ctx);
                continue; // Try again from start of animation
            }
            return 0; // Animation finished
        } else if (separator == 0x21) { // Extension Introducer
            gif_read_ext(ctx);
        } else if (separator == 0x2C) { // Image Descriptor
            break; // Found an image, proceed to decode
        } else {
            gif_report_error(ctx, GIF_ERROR_BAD_FILE, "Unexpected byte in GIF stream.");
            return -1;
        }
    }

    if (ctx->current_pos >= ctx->gif_size) {
        return 0; // No more frames
    }

    ctx->frame_x_off = gif_read_u16_le(ctx->gif_data + ctx->current_pos); gif_skip_bytes_internal(ctx, 2);
    ctx->frame_y_off = gif_read_u16_le(ctx->gif_data + ctx->current_pos); gif_skip_bytes_internal(ctx, 2);
    ctx->frame_width = gif_read_u16_le(ctx->gif_data + ctx->current_pos); gif_skip_bytes_internal(ctx, 2);
    ctx->frame_height = gif_read_u16_le(ctx->gif_data + ctx->current_pos); gif_skip_bytes_internal(ctx, 2);

    // Validate frame dimensions
    if (ctx->frame_width == 0 || ctx->frame_height == 0) {
        gif_report_error(ctx, GIF_ERROR_INVALID_FRAME_DIMENSIONS, "Frame has zero width or height.");
        return -1;
    }
    if (ctx->frame_x_off + ctx->frame_width > ctx->canvas_width ||
        ctx->frame_y_off + ctx->frame_height > ctx->canvas_height)
    {
        gif_report_error(ctx, GIF_ERROR_INVALID_FRAME_DIMENSIONS, "Frame extends beyond canvas boundaries.");
        return -1;
    }

    uint8_t fisrz = gif_read_byte_internal(ctx);
    ctx->ucGIFBits = fisrz;

    if (fisrz & 0x80) { // Local Color Table Flag
        int lct_size = 1 << ((fisrz & 0x07) + 1);
        if (lct_size > GIF_MAX_COLORS) {
            gif_report_error(ctx, GIF_ERROR_UNSUPPORTED_COLOR_DEPTH, "Local Color Table size exceeds GIF_MAX_COLORS.");
            return -1;
        }
        if (gif_read_bytes_internal(ctx, ctx->local_palette_colors, lct_size * 3) < lct_size * 3) {
            gif_report_error(ctx, GIF_ERROR_EARLY_EOF, "Early EOF while reading Local Color Table.");
            return -1;
        }
        ctx->active_palette_colors = ctx->local_palette_colors;
    } else {
        ctx->active_palette_colors = ctx->global_palette_colors;
    }

    ctx->lzw_code_start_size = gif_read_byte_internal(ctx);

    int decode_result = gif_decode_lzw(ctx, frame_buffer);
    if (decode_result != GIF_SUCCESS) {
        gif_report_error(ctx, decode_result, "LZW decoding failed for frame.");
        return -1;
    }

    *delay_ms = ctx->frame_delay_ms;
    return 1;
}

void gif_rewind(GIF_Context *ctx) {
    if (ctx) {
        ctx->current_pos = ctx->anim_start_pos;
        ctx->lzw_end_of_frame = 0;
        ctx->lzw_read_offset = 0;
        ctx->lzw_data_size = 0;
    }
}

void gif_close(GIF_Context *ctx) {
    if (ctx) {
        memset(ctx, 0, sizeof(GIF_Context));
    }
}

void gif_set_error_callback(GIF_Context *ctx, GIF_ErrorCallback callback) {
    if (ctx) {
        ctx->error_callback = callback;
    }
}

#endif // GIF_IMPLEMENTATION

#endif // GIF_H

