/*
 * utf8.cpp
 * Copyright (C) 2023 Kovid Goyal <kovid at kovidgoyal.net>
 *
 * Distributed under terms of the GPL3 license.
 */

#include "charsets.h"
#include "data-types.h"
#include "utf8.h"
#include "../3rdparty/simdutf/simdutf.h"


static bool
utf8_decode_to_first_error(const uint8_t *src, size_t src_sz, bool is_complete, uint32_t *dest, size_t *num_written, size_t *num_read) {
    auto result = simdutf::convert_utf8_to_utf32_with_errors((const char*)src, src_sz, (char32_t*)dest);
    switch (result.error) {
        case simdutf::error_code::SUCCESS:
            *num_written = result.count;
            *num_read = src_sz;
            return true;
        case simdutf::error_code::TOO_SHORT:
            if (is_complete) return false;
            while (result.count && (src[result.count-1] & 0b11000000) == 0b10000000) result.count--;
            result = simdutf::convert_utf8_to_utf32_with_errors((const char*)src, result.count, (char32_t*)dest);
            if (result.error == simdutf::error_code::SUCCESS) {
                *num_written = result.count;
                *num_read = src_sz;
                return true;
            }
            break;
        default:
            if (result.count > 31) {  // retry for large input upto the failing char
                result = simdutf::convert_utf8_to_utf32_with_errors((const char*)src, result.count, (char32_t*)dest);
                if (result.error == simdutf::error_code::SUCCESS) {
                    *num_written = result.count;
                    *num_read = src_sz;
                    return true;
                }
            }
            break;
    }
    *num_written = 0; *num_read = 0;
    return false;
}


size_t
utf8_decode_with_replace(const uint8_t *src, size_t src_sz, bool is_complete, uint32_t *dest, size_t *num_read) {
    size_t num_written;
    if (utf8_decode_to_first_error(src, src_sz, is_complete, dest, &num_written, num_read)) return num_written;
    uint32_t prev = UTF8_ACCEPT, state = UTF8_ACCEPT, codep = 0;
    src += *num_read;
    for (const uint8_t *limit = src + src_sz; src < limit; src++, (*num_read)++) {
        switch (decode_utf8(&state, &codep, *src)) {
            case UTF8_ACCEPT:
                dest[num_written++] = codep;
                // we could retry the SIMD decode after dealing with the error, but I cant be bothered
                break;
            case UTF8_REJECT: {
                bool prev_was_accept = prev == UTF8_ACCEPT;
                dest[num_written++] = 0xfffd;
                state = UTF8_ACCEPT; prev = UTF8_ACCEPT; codep = 0;
                if (!prev_was_accept) {
                    src--; (*num_read)--;
                    continue; // so that prev is correct
                }
            } break;
        }
        prev = state;
    }
    return num_written;
}
