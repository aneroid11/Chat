#include <iostream>
#include <vector>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
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
    }

    while (true) {}

    close(socketFd);
    return 0;
}
