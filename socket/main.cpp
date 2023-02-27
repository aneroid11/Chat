#include <iostream>
#include <vector>
#include <cstring>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

const int PORT = 8080;
const int MAX_PACKET_LEN = 256;

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

int main()
{
    const int socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd < 0)
    {
        perror("failed to create socket");
        exit(EXIT_FAILURE);
    }

    sockaddr_in serverAddress {};
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);

    // to bind or not to bind?
    if (getUserChoice({"open a connection", "connect to a running client"}) == 0)
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

        char data[MAX_PACKET_LEN] = "hello world";
        const size_t data_size = strlen(data) + 1;
        sendto(socketFd, data, data_size, MSG_CONFIRM,
               (const sockaddr*)&serverAddress, sizeof(serverAddress));
        std::cout << "sent 'hello world' to other client\n";
    }

    while (true)
    {
        char buffer[MAX_PACKET_LEN];
        sockaddr_in clientAddress {};
        memset(&clientAddress, 0, sizeof(clientAddress));
        socklen_t addrLen;
        recvfrom(socketFd, buffer, MAX_PACKET_LEN, MSG_WAITALL,
                 (sockaddr*)&clientAddress, &addrLen);

        std::cout << "received data: " << buffer << "\n";

        std::cout << "received from: ";
    }

    close(socketFd);
    return 0;
}
