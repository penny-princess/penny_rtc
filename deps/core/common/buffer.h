//
// Created by penny_developers@outlook.com on 2024-07-08.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
//

#pragma once
#include "pch.h"

typedef char *Buffer;

struct _BufferHrd {
    int len;  // length of the string
    int free; // amount of free space at the end
    char buf[];  // string buffer
};

static inline _BufferHrd *_get_hdr(Buffer buffer) {
    return reinterpret_cast<_BufferHrd*>(buffer - sizeof(_BufferHrd));
}

// Create a new SDS string
Buffer buffer_new_len(const void *init, int initlen);

Buffer buffer_new(const char *init);

Buffer buffer_empty();

void buffer_free(Buffer s);

int buffer_len(const Buffer s);

int buffer_avail(const Buffer s);

Buffer buffer_dup(const Buffer s);

Buffer buffer_append_len(Buffer s, const void *t, int len);

void buffer_set_len(Buffer s, int incr);

Buffer buffer_append(Buffer s, const char *t);

Buffer buffer_cpy_len(Buffer s, const char *t, int len);

Buffer buffer_cpy(Buffer s, const char *t);

Buffer buffer_allocate(Buffer s, size_t addlen);