/*
 * Copyright (C) 2023 Kovid Goyal <kovid at kovidgoyal.net>
 *
 * Distributed under terms of the GPL3 license.
 */

#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

size_t
utf8_decode_with_replace(const uint8_t *src, size_t src_sz, bool is_complete, uint32_t *dest, size_t *num_read);
