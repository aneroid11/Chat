//
// Created by lucky on 3/9/23.
//

#ifndef SOCKET_MESSAGE_H
#define SOCKET_MESSAGE_H

#include <string>
#include <chrono>

class Message {
public:
    std::string sender, receiver, contents;
    std::string timestamp;
};

#endif //SOCKET_MESSAGE_H
