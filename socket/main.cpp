#include <iostream>
#include <vector>
#include <random>
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
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(1, 1000);

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
        // send a message
        char data[MAX_PACKET_LEN] = "hello world";
        const size_t data_size = strlen(data) + 1;
        sendto(socketFd, data, data_size, MSG_CONFIRM,
               (const sockaddr*)&serverAddress, sizeof(serverAddress));
        std::cout << "sent 'hello world' to other client\n";
    }

    int msg_num = 0;
    const int client_id = dist(mt);

    while (true)
    {
        char buffer[MAX_PACKET_LEN];
        sockaddr_in clientAddress {};
        memset(&clientAddress, 0, sizeof(clientAddress));
        socklen_t addrLen;
        recvfrom(socketFd, buffer, MAX_PACKET_LEN, MSG_WAITALL,
                 (sockaddr*)&clientAddress, &addrLen);

        std::cout << "received data: " << buffer << "\n";

        std::string msg = "hello world ";
        msg += std::to_string(msg_num) + " from " + std::to_string(client_id);
        msg_num++;

        char data[MAX_PACKET_LEN] = "hello world 2";
        const size_t data_size = strlen(data) + 1;

        // sockaddr_in sendToAddress = weAreFirst ? clientAddress : serverAddress;

        sendto(socketFd, msg.c_str(), msg.size(), MSG_CONFIRM,
               (const sockaddr*)&clientAddress, addrLen);
        /*sendto(socketFd, data, data_size, MSG_CONFIRM,
               (const sockaddr*)&clientAddress, addrLen);*/
        //std::cout << "sent 'hello world 2' to other client\n";

        usleep(100);
    }

    close(socketFd);
    return 0;
}
