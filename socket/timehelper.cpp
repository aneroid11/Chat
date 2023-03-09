//
// Created by lucky on 3/9/23.
//

#include "timehelper.h"

std::string timePointToString(const std::chrono::system_clock::time_point& timePoint)
{
    //std::chrono::system_clock::time_point timePoint = std::chrono::system_clock::now();
    std::time_t tpTimer = std::chrono::system_clock::to_time_t(timePoint);
    std::tm tpTm = *std::localtime(&tpTimer);
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%c", &tpTm);
    return buffer;
}
