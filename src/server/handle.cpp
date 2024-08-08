//
// Created by penny_developers@outlook.com on 2024-08-02.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
// 


#include "handle.h"

namespace penny {

    Handle::~Handle() {

    }

    void Handle::connect() {

    }

    void Handle::error() {

    }

    int Handle::handle0(const std::string &header,const std::string &body) {
        LOG(INFO) << "header is:" << header << ", body is:" << body;

        return 0;
    }


}
