

#pragma once

#include <pch.h>

namespace penny {

    const int XHEAD_SIZE = 36;
    const int XHEAD_MAGIC_NUM = 0xfb202202;

    struct protocol_header_t {
        int id;
        int version;
        int log_id;
        int magic_num;
        int reserved;
        int body_len;
        char provider[16];
    };

}



