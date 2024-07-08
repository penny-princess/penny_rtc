//
// Created by penny_developers@outlook.com on 2024-07-08.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
//

#include "buffer.h"

Buffer buffer_new_len(const void *init, int initlen) {
    struct _BufferHrd *sh = (struct _BufferHrd *) malloc(sizeof(struct _BufferHrd) + initlen + 1);
    if (sh == nullptr) return nullptr;
    sh->len = initlen;
    sh->free = 0;
    if (initlen && init) {
        memcpy(sh->buf, init, initlen);
    }
    sh->buf[initlen] = '\0';
    return sh->buf;
}

Buffer buffer_new(const char *init) {
    int initlen = (init == nullptr) ? 0 : strlen(init);
    return buffer_new_len(init, initlen);
}

Buffer buffer_empty() {
    return buffer_new("");
}

void buffer_free(Buffer s) {
    if (s == nullptr) return;
    free(_get_hdr(s));
}

int buffer_len(const Buffer s) {
    return _get_hdr(s)->len;
}

int buffer_avail(const Buffer s) {
    return _get_hdr(s)->free;
}

Buffer buffer_dup(const Buffer s) {
    return buffer_new_len(s, buffer_len(s));
}

Buffer buffer_allocate(Buffer s, size_t addlen) {
    _BufferHrd* sh = _get_hdr(s);
    size_t free = sh->free;
    if (free >= addlen) return s;

    size_t len = sh->len;
    size_t newlen = len + addlen;
    if (newlen < len) return nullptr; // Overflow

    // Allocate more space
    _BufferHrd* newsh = reinterpret_cast<_BufferHrd*>(new char[sizeof(_BufferHrd) + newlen + 1]);
    memcpy(newsh->buf, sh->buf, len);
    newsh->len = len;
    newsh->free = newlen - len;

    buffer_free(s);
    return (Buffer)newsh->buf;
}

Buffer buffer_append_len(Buffer s, const void *t, int len) {
    struct _BufferHrd *sh = _get_hdr(s);
    int curlen = sh->len;
    if (sh->free < len) {
        sh = (struct _BufferHrd *) realloc(sh, sizeof(struct _BufferHrd) + curlen + len + 1);
        if (sh == nullptr) return nullptr;
        s = sh->buf;
        sh->free = sh->free + len;
    }
    memcpy(s + curlen, t, len);
    sh->len = curlen + len;
    sh->free -= len;
    s[curlen + len] = '\0';
    return s;
}


void buffer_set_len(Buffer s, int incr) {
    struct _BufferHrd *sh = _get_hdr(s);
    assert(sh->free >= incr);
    sh->len += incr;
    sh->free -= incr;
    s[sh->len] = '\0';
}

Buffer buffer_append(Buffer s, const char *t) {
    return buffer_append_len(s, t, strlen(t));
}

Buffer buffer_cpy_len(Buffer s, const char *t, int len) {
    struct _BufferHrd *sh = _get_hdr(s);
    if (sh->free + sh->len < len) {
        sh = (struct _BufferHrd *) realloc(sh, sizeof(struct _BufferHrd) + len + 1);
        if (sh == nullptr) return nullptr;
        s = sh->buf;
        sh->free = len - sh->len;
    }
    memcpy(s, t, len);
    s[len] = '\0';
    sh->free = sh->free + sh->len - len;
    sh->len = len;
    return s;
}


Buffer buffer_cpy(Buffer s, const char *t) {
    return buffer_cpy_len(s, t, strlen(t));
}
