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

int getIntInput(const std::string& prompt)
{
    std::cout << prompt;
    int input;
    std::cin >> input;

    while (!std::cin)
    {
        std::cerr << "Invalid input!\n";
        std::cin >> input;
    }

    std::cin.get();
    return input;
}

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

    while (true)
    {
        int choice = getUserChoice({"show incoming messages", "send a message"});

        if (choice == 0)
        {
            std::string msg;
            sockaddr_in addrBuf {};
            socklen_t lenBuf;

            while (receiveMsg(msg, socketFd, addrBuf, lenBuf) >= 0)
            {
                std::cout << "Message from " << getIpPortFromSockaddr(addrBuf) << ":\n";
                std::cout << msg << "\n";
            }
        }
        else
        {
            const int port = getIntInput("Enter the port of the other client: ");

            sockaddr_in otherAddress {};
            memset(&otherAddress, 0, sizeof(otherAddress));
            otherAddress.sin_family = AF_INET;
            otherAddress.sin_addr.s_addr = INADDR_ANY;
            otherAddress.sin_port = htons(port);

            std::string message;
            std::cout << "Enter the message: ";
            std::getline(std::cin, message);

            sendMsg(message, socketFd, otherAddress, sizeof(otherAddress));
            std::cout << "Message sent.\n";
        }
    }

    close(socketFd);
    return 0;
}
