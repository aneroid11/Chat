#include <iostream>
//#include <sstream>
#include <thread>
#include <functional>
#include <memory>
#include <vector>
#include <list>
#include <cstring>
#include <chrono>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "timehelper.h"
#include "message.h"

const int MAX_PACKET_LEN = 256;
const int TIMEOUT_MS = 100;

const int REQUEST_FOR_CONNECTION = 3;
const int ACCEPT_REQUEST = 1;
const int DECLINE_REQUEST = 2;

int getUserChoice(const std::vector<std::string>& options)
{
    const size_t numOptions = options.size();

    for (int i = 0; i < numOptions; i++)
    {
        std::cout << i << " - " << options[i] << "\n";
    }

    std::string optionStr;
    int option;

    while (true)
    {
        std::cout << "Choose an option: ";
        std::cin >> optionStr;

        try
        {
            option = std::stoi(optionStr);
        }
        catch (const std::exception& e)
        {
            std::cout << "Invalid input!\n";
            continue;
        }
        if (option < 0 || option >= numOptions)
        {
            std::cout << "Option index out of range!\n";
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

    while (!(std::cin >> input))
    {
        std::cerr << "Invalid input! Please try again: ";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<int>::max(), '\n');
    }

    std::cin.get();
    return input;
}

std::list<Message> readMessageHistory(const std::string& clientName)
{
    std::list<Message> ret;
    ret.push_back(Message {"Man 1", "hello", "45:88"});
    ret.push_back(Message {"Man 2", "hello", "45:11"});
    ret.push_back(Message {"Man 3", "hello", "45:900"});
    return ret;
}

void printMessageHistory(const std::list<Message>& history)
{
    std::cout << "\nMESSAGE HISTORY:\n\n";

    for (auto& msg : history)
    {
        std::cout << "Message from " << msg.sender << " at " << msg.timestamp << "\n";
        std::cout << msg.contents << "\n\n";
    }
}

void receiveMessages(const std::string& clientName,
                     const std::string& otherClientName,
                     const sockaddr_in& otherClientAddr,
                     const socklen_t otherClientSocklen,
                     const int socketFd)
{
    std::cout << "I am receiving messages and displaying them\n";
}

void talk(const std::string& clientName,
          const std::string& otherClientName,
          const sockaddr_in& otherClientAddr,
          const socklen_t otherClientSocklen,
          const int socketFd)
{
    std::cout << "You can send your messages now. Enter !exit to finish the conversation\n";

    std::thread recMsgs(std::bind(receiveMessages,
                                  std::cref(clientName),
                                  std::cref(otherClientName),
                                  std::cref(otherClientAddr),
                                  otherClientSocklen,
                                  socketFd));

    while (true)
    {
        std::string msgContents;
        std::getline(std::cin, msgContents);

        if (msgContents == "!exit")
        {
            break;
        }

        sendMsg(msgContents, socketFd, otherClientAddr, otherClientSocklen);
    }

    recMsgs.join();
}

int main()
{
    std::string clientName;
    std::cout << "Enter your name: ";
    std::cin >> clientName;

    printMessageHistory(readMessageHistory(clientName));

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
    ourAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
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
        const int choice = getUserChoice({
            "check for connection requests", "send a connection request"
        });

        if (choice == 0)
        {
            sockaddr_in sockaddrIn {};
            socklen_t socklen;
            std::string otherClientName;

            bool hasRequests = false;

            while (true)
            {
                std::string msg;
                if (receiveMsg(msg, socketFd, sockaddrIn, socklen) < 0)
                {
                    break;
                }

                // что-то таки пришло. это что-то надо обработать.
                if (msg[0] == (char)REQUEST_FOR_CONNECTION)
                {
                    hasRequests = true;
                    otherClientName = msg.substr(1);

                    if (clientName == otherClientName)
                    {
                        std::cout << "You cannot connect to yourself!\n";
                        std::string response = std::string(1, (char)DECLINE_REQUEST);
                        sendMsg(response, socketFd, sockaddrIn, socklen);
                    }
                    else
                    {
                        std::cout << "You have an incoming connection request from " << otherClientName << "\n";
                        const int acceptOrDecline = getUserChoice({"accept", "decline"});

                        if (acceptOrDecline == 0)
                        {
                            std::string response = std::string(1, (char)ACCEPT_REQUEST);
                            response += clientName;
                            sendMsg(response, socketFd, sockaddrIn, socklen);

                            std::cout << "You are connected to " << otherClientName << "\n";

                            // что передать в talk?
                            // наше имя
                            // имя того, с кем общаемся
                            // наш адрес
                            // адрес того, с кем общаемся
                            //talk(clientName, otherClientName, sockaddrIn);
                            talk(clientName, otherClientName, sockaddrIn, socklen, socketFd);
                            break;
                        }
                        else
                        {
                            std::string response = std::string(1, (char)DECLINE_REQUEST);
                            sendMsg(response, socketFd, sockaddrIn, socklen);
                            std::cout << "You declined the request.\n";
                            continue;
                        }
                    }
                }
            }
            if (!hasRequests)
            {
                std::cout << "\nNo requests.\n\n";
            }
        }
        else
        {
            const int otherPort = getIntInput("Enter the port of the other client: ");
            sockaddr_in otherAddress {};
            memset(&otherAddress, 0, sizeof(otherAddress));
            otherAddress.sin_family = AF_INET;
            otherAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
            otherAddress.sin_port = htons(otherPort);

            std::string msg = std::string(1, (char)REQUEST_FOR_CONNECTION);
            msg += clientName;
            sendMsg(msg, socketFd, otherAddress, sizeof(otherAddress));

            std::cout << "The connection request was sent. Please wait...\n";

            sockaddr_in sockaddrIn {};
            socklen_t socklen;
            std::string otherClientName;

            // wait for accept or decline
            while (true)
            {
                std::string response;

                if (receiveMsg(response, socketFd, sockaddrIn, socklen) < 0)
                {
                    continue;
                }

                //std::cout << getIpPortFromSockaddr(sockaddrIn) << "\n";
                //std::cout << getIpPortFromSockaddr(otherAddress) << "\n";
                // if the response was sent from the same client
                if (getIpPortFromSockaddr(sockaddrIn) == getIpPortFromSockaddr(otherAddress))
                {
                    if (response[0] == (char)ACCEPT_REQUEST)
                    {
                        // get the other client name
                        otherClientName = response.substr(1);
                        std::cout << "Your request was accepted by " << otherClientName << ".\n";
                        talk(clientName, otherClientName, sockaddrIn, socklen, socketFd);
                        break;
                    }
                    else if (response[0] == (char)DECLINE_REQUEST)
                    {
                        std::cout << "Your request was declined.\n";
                        break;
                    }
                }
                // else - ignore this message
            }
        }
    }

    close(socketFd);
    return 0;
}
