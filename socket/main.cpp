#include <iostream>
#include <vector>
#include <random>
#include <cstring>
#include <chrono>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

const int PORT = 8080;
const int MAX_PACKET_LEN = 256;
const int TIMEOUT_MS = 100;

int getUserChoice(const std::vector<std::string>& options)
{
    const size_t numOptions = options.size();

    for (int i = 0; i < numOptions; i++)
    {
        std::cout << i << " - " << options[i] << "\n";
    }

    int option;

    while (true)
    {
        std::cout << "Choose an option: ";
        std::cin >> option;

        if (!std::cin || option < 0 || option >= numOptions)
        {
            std::cout << "Invalid input!\n";
            continue;
        }
        break;
    }

    return option;
}

std::string getIpPortFromSockaddr(const sockaddr_in* sockAddr)
{
    char ip[INET_ADDRSTRLEN];
    uint16_t port = htons(sockAddr->sin_port);
    inet_ntop(AF_INET, &sockAddr->sin_addr, ip, sizeof(ip));
    return std::string(ip) + ":" + std::to_string(port);
}

std::string currTimeMcs()
{
    //static auto startingPoint = std::chrono::high_resolution_clock::now();

    auto now = std::chrono::high_resolution_clock::now();
    return std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count());
}

ssize_t sendMsg(const std::string& msg, const int socketFd, const sockaddr_in& sockAddr, const socklen_t addrLen)
{
    return sendto(socketFd, msg.c_str(), msg.size() + 1, MSG_CONFIRM,
                (const sockaddr*)&sockAddr, addrLen);
}

ssize_t receiveMsg(std::string& msg, const int socketFd, sockaddr_in& sockAddr, socklen_t& addrLen)
{
    memset(&sockAddr, 0, sizeof(sockAddr));
    char buffer[MAX_PACKET_LEN];
    addrLen = sizeof(sockaddr_in);
    ssize_t code = recvfrom(socketFd, buffer, MAX_PACKET_LEN, MSG_WAITALL,
                        (sockaddr*)&sockAddr, &addrLen);
    msg = buffer;
    return code;
}

void sendWithValidation(const std::string& msg, const int socketFd, const sockaddr_in& sockAddr, const socklen_t addrLen)
{
    std::cout << "sendWithValidation() start\n";
    bool okNotReceived = false;

    do
    {
        std::cout << "sendMsg()\n";
        if (sendMsg(msg, socketFd, sockAddr, addrLen) < 0)
        {
            continue;
        }

        std::string recMsg;
        sockaddr_in addr {};
        socklen_t len;
        std::cout << "before receiveMsg()\n";
        okNotReceived = receiveMsg(recMsg, socketFd, addr, len) < 0 || recMsg != "ok";
        std::cout << "after receiveMsg()\n";
    } while (okNotReceived);

    std::cout << "sendMsg(ok)\n";
    sendMsg("ok", socketFd, sockAddr, addrLen);

    std::cout << "sendWithValidation() finished\n";
}

void receiveWithValidation(std::string& msg, const int socketFd, sockaddr_in& sockAddr, socklen_t& sockLen)
{
    while (true)
    {
        while (receiveMsg(msg, socketFd, sockAddr, sockLen) < 0) {}

        if (msg == "ok") // not a message
        {
            sendMsg("ok", socketFd, sockAddr, sockLen);
            continue;
        }
        break;
    }

    bool okNotReceived = false;

    do
    {
        if (sendMsg("ok", socketFd, sockAddr, sockLen) < 0)
        {
            continue;
        }

        std::string buffer;
        okNotReceived = receiveMsg(buffer, socketFd, sockAddr, sockLen) < 0 || buffer != "ok";
    } while (okNotReceived);
}

int main()
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(1, 1000);

    const int socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd < 0)
    {
        perror("failed to create socket");
        exit(EXIT_FAILURE);
    }

    timeval tv {0, TIMEOUT_MS * 1000};
    if (setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        perror("failed to set timeout option");
        exit(EXIT_FAILURE);
    }

    sockaddr_in serverAddress {};
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);

    // to bind or not to bind?
    bool bindPort = getUserChoice({"open a connection", "connect to a running client"}) == 0;

    if (bindPort)
    {
        if (bind(socketFd, (const sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
        {
            perror("Failed to bind the socket");
            exit(EXIT_FAILURE);
        }

        std::cout << "Bound the socket successfully\n";
        std::cout << "Waiting for connections...\n";
    }
    else
    {
        std::cout << "Trying to connect...\n";
    }

    if (!bindPort)
    {
        //sendMsg("hello world", socketFd, serverAddress, sizeof(serverAddress));
        sendWithValidation("first message", socketFd, serverAddress, sizeof(serverAddress));
    }

    int msg_num = 0;
    const int client_id = dist(mt);

    while (true)
    {
        std::cout << currTimeMcs() << " receiveMsg...\n";
        sockaddr_in clientAddress {};
        socklen_t addrLen;
        std::string msg;
        receiveWithValidation(msg, socketFd, clientAddress, addrLen);

        std::cout << currTimeMcs() << " receiveMsg finished.\n";

        std::cout << currTimeMcs() << " received data: " << msg << "\n";

        std::string msgToSend = "hello world ";
        msgToSend += std::to_string(msg_num) + " from " + std::to_string(client_id);
        msg_num++;

        std::cout << currTimeMcs() << " sendMsg...\n";
        sendMsg(msgToSend, socketFd, clientAddress, addrLen);
        std::cout << currTimeMcs() << " sendMsg finished.\n";
    }

    close(socketFd);
    return 0;
}
