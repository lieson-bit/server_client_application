#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <string>
#include <algorithm>
#include <thread>
#include <map>

#define BUFFER_SIZE 512       // Buffer size for receiving data
#define SERVER_IP "172.18.182.131" // Define server IP address

// Function to count occurrences of a word in a file
int countWordOccurrences(const std::string &filename, const std::string &word) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return -1;
    }

    std::string line;
    int count = 0;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;
        while (iss >> token) {
            std::string tokenLower = token;
            std::string wordLower = word;
            std::transform(tokenLower.begin(), tokenLower.end(), tokenLower.begin(), ::tolower);
            std::transform(wordLower.begin(), wordLower.end(), wordLower.begin(), ::tolower);

            if (tokenLower == wordLower) {
                ++count;
            }
        }
    }
    file.close();
    return count;
}

// Function to handle a client request
void handleClientRequest(int serverSocket, sockaddr_in clientAddr, const std::string &request) {
    std::istringstream iss(request);
    std::string word, filename;

    if (!(iss >> word >> filename)) {
        std::cerr << "Invalid message format!" << std::endl;
        return;
    }

    int count = countWordOccurrences(filename, word);
    std::map<std::string, int> wordCountMap;

    if (count == -1) {
        std::cerr << "Error: File not found!" << std::endl;
    } else {
        wordCountMap[word] = count;
    }

    std::ostringstream response;
    bool first = true;
    for (const auto &entry : wordCountMap) {
        if (!first) {
            response << std::endl;
        }
        response << entry.first << " " << entry.second;
        first = false;
    }

    ssize_t bytesSent = sendto(serverSocket, response.str().c_str(), response.str().size(), 0,
                               (struct sockaddr *)&clientAddr, sizeof(clientAddr));
    if (bytesSent < 0) {
        std::cerr << "Error sending data to client!" << std::endl;
    }
}

// Function to handle incoming client messages
void handleClient(int serverSocket) {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    char buffer[BUFFER_SIZE];

    while (true) {
        ssize_t bytesReceived = recvfrom(serverSocket, buffer, BUFFER_SIZE - 1, 0,
                                         (struct sockaddr *)&clientAddr, &clientAddrLen);

        if (bytesReceived < 0) {
            std::cerr << "Error receiving data!" << std::endl;
            continue;
        }

        buffer[bytesReceived] = '\0';
        std::cout << "Received: " << buffer << " from client "
                  << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;

        std::thread(handleClientRequest, serverSocket, clientAddr, std::string(buffer)).detach();
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: server <PORT>" << std::endl;
        return 1;
    }

    int serverPort = std::stoi(argv[1]);

    int serverSocket;
    struct sockaddr_in serverAddr;

    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error creating socket!" << std::endl;
        return 1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP); // Bind to the specified IP address
    serverAddr.sin_port = htons(serverPort);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket!" << std::endl;
        close(serverSocket);
        return 1;
    }

    std::cout << "Server is running on IP: " << SERVER_IP << " and waiting for messages on port "
              << serverPort << "..." << std::endl;

    handleClient(serverSocket);

    close(serverSocket);
    return 0;
}
