

#pragma once

#include <pch.h>

namespace penny {
    const int HEADER_SIZE = 40;
    const int PROTOCOL_CHECK_NUM = 0x0990908;

    struct protocol_header_t {
        int32_t id;                     //4
        int32_t version;                //4
        int32_t log_id;                 //4
        int32_t protocol_check_num;     //4
        int32_t reserved;               //4
        int32_t body_len;               //4
        char provider[16];              //16
    };

}



