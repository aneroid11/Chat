//
// Created by lucky on 3/9/23.
//

#ifndef SOCKET_TIMEHELPER_H
#define SOCKET_TIMEHELPER_H

#include <chrono>
#include <string>

std::string timePointToString(const std::chrono::system_clock::time_point& timePoint);

#endif //SOCKET_TIMEHELPER_H
