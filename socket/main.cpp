#include <iostream>
#include <vector>
#include <random>
#include <cstring>
#include <chrono>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

//const int PORT = 8080;
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

    std::cin.get();
    return option;
}

std::string getIpPortFromSockaddr(const sockaddr_in& sockAddr)
{
    char ip[INET_ADDRSTRLEN];
    uint16_t port = htons(sockAddr.sin_port);
    inet_ntop(AF_INET, &sockAddr.sin_addr, ip, sizeof(ip));
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

ssize_t sendMsgAndReceiveOk(const std::string& msg, const int socketFd, const sockaddr_in& sockAddr, const socklen_t addrLen)
{
    std::cout << "sendMsgAndReceiveOk()\n";
    return 0;
}

/*
void sendWithValidation(const std::string& msg, const int socketFd, const sockaddr_in& sockAddr, const socklen_t addrLen)
{
    bool okNotReceived = false;

    do
    {
        if (sendMsg(msg, socketFd, sockAddr, addrLen) < 0)
        {
            continue;
        }
        std::cout << "sendWithValidation(): sent the message\n";

        std::string recMsg;
        sockaddr_in addr {};
        socklen_t len;
        okNotReceived = receiveMsg(recMsg, socketFd, addr, len) < 0 || recMsg != "ok";
    } while (okNotReceived);

    std::cout << "sendWithValidation(): received ok\n";

    sendMsg("ok", socketFd, sockAddr, addrLen);

    std::cout << "sendWithValidation(): sent ok\n";
}

void receiveWithValidation(std::string& msg, const int socketFd, sockaddr_in& sockAddr, socklen_t& sockLen)
{
    while (true)
    {
        while (receiveMsg(msg, socketFd, sockAddr, sockLen) < 0) {}

        if (msg == "ok") // not a message
        {
            while (sendMsg("ok", socketFd, sockAddr, sockLen) < 0) {}
            continue;
        }
        break;
    }
    std::cout << "receiveWithValidation(): received a message\n";

    bool okNotReceived = false;

    do
    {
        if (sendMsg("ok", socketFd, sockAddr, sockLen) < 0)
        {
            std::cout << "receiveWithValidation(): cannot send ok\n";
            continue;
        }
        std::cout << "receiveWithValidation(): sent ok\n";

        std::string buffer;
        okNotReceived = receiveMsg(buffer, socketFd, sockAddr, sockLen) < 0 || buffer != "ok";
    } while (okNotReceived);

    std::cout << "receiveWithValidation(): received ok\n";
}*/

int main()
{
    /*std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(1, 1000);*/

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

    sockaddr_in ourAddress {};
    memset(&ourAddress, 0, sizeof(ourAddress));
    ourAddress.sin_family = AF_INET;
    ourAddress.sin_addr.s_addr = INADDR_ANY;
    ourAddress.sin_port = htons(0); // any available port

    if (bind(socketFd, (const sockaddr*)&ourAddress, sizeof(ourAddress)) < 0)
    {
        perror("Failed to bind the socket");
        exit(EXIT_FAILURE);
    }

    sockaddr_in addr {};
    socklen_t len = sizeof(sockaddr_in);

    if (getsockname(socketFd, (sockaddr*)&addr, &len) < 0)
    {
        perror("Failed to get socket name");
        exit(EXIT_FAILURE);
    }
    const int ourPort = htons(addr.sin_port);
    std::cout << "Bound to port " << ourPort << "\n";

    sockaddr_in otherAddr {};
    socklen_t otherLen;

    while (true)
    {
        int choice = getUserChoice({"check for connections", "connect to the other client"});
        if (choice == 0)
        {
            // просмотреть, есть ли отправленные на наш сокет запросы на коннект
            // а сам коннект будет состоять в том, что каждые 30 секунд клиент отправляет
            // сообщение, что он ещё живой (если он не отправлял сообщений последние 30 секунд)
            // если ответ ok на сообщение о живости не пришёл (10 раз подряд), то тот клиент отключился
            // если соединение потеряно, то надо вернуться в состояние WAITING
            // ...состояние? это конечный автомат должен быть?
            std::cout << "Check for connections\n";

            sockaddr_in someAddr {};
            memset(&someAddr, 0, sizeof(someAddr));
            someAddr.sin_family = AF_INET;
            someAddr.sin_addr.s_addr = INADDR_ANY;
            someAddr.sin_port = htons(8080);
            ssize_t result = sendMsg("msg", socketFd, someAddr, sizeof(sockaddr_in));

            // можно спокойно отправлять несуществующему клиенту, и это не вызовет ошибку.
            std::cout << "result of sending: " << result << "\n";
            /*std::string msg;

            if (receiveMsg(msg, socketFd, otherAddr, otherLen) < 0)
            {
                // no connections
                std::cout << "No connection requests.\n";
                continue;
            }
            else
            {
                // send 'ok' to the first, send 'busy' to others
                sendMsg("ok", socketFd, otherAddr, otherLen);

                sockaddr_in buffer {};
                socklen_t bufferLen;
                while (receiveMsg(msg, socketFd, buffer, bufferLen) >= 0)
                {
                    sendMsg("busy", socketFd, otherAddr, otherLen);
                }

                std::cout << "Connected to " << getIpPortFromSockaddr(otherAddr) << "\n";
                break;
            }*/
        }
        else
        {
            std::cout << "Connect to a client online\n";
            /*int port;
            std::cout << "Enter the port of the other client: ";
            do
            {
                std::cin >> port;
            } while (!std::cin);

            memset(&otherAddr, 0, sizeof(otherAddr));
            otherAddr.sin_family = AF_INET;
            otherAddr.sin_addr.s_addr = INADDR_ANY;
            otherAddr.sin_port = htons(port);

            std::cout << "Trying to connect...\n";
            sendMsg("pls connect", socketFd, otherAddr, sizeof(otherAddr));*/
        }
    }

    /*
    int choice = getUserChoice({"send a message", "show incoming messages"});
    if (choice == 0)
    {

    }
    else
    {

    }*/

    /*
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
    }*/

    /*
    if (!bindPort)
    {
        std::string msg;
        std::cout << "Enter your message: ";
        std::getline(std::cin, msg);
        sendWithValidation(msg, socketFd, ourAddress, sizeof(ourAddress));
    }

    while (true)
    {
        sockaddr_in clientAddress {};
        socklen_t addrLen;
        std::string msg;
        receiveWithValidation(msg, socketFd, clientAddress, addrLen);

        std::cout << "Received message: " << msg << " from " << getIpPortFromSockaddr(clientAddress) << "\n";

        std::string msgToSend;
        std::cout << "Enter your message: ";
        std::getline(std::cin, msgToSend);
        sendWithValidation(msgToSend, socketFd, clientAddress, addrLen);
    }*/

    close(socketFd);
    return 0;
}
