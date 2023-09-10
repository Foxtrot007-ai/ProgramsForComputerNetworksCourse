#include <iostream>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <deque>
#include <utility>
#include <errno.h>

class Transport{
    private:
        int sockfd;
        int filefd;
        struct sockaddr_in serverAddress;
        std::string ip;
        int port; 
        std::string fileName; 
        ssize_t size;

        int bytesReceived;
        const int packetSize = 1000;
        const int windowSize = 128;
        std::deque<std::pair<ssize_t, char*>> window;
        
        void validateInput();
        void init();
        void sendPacket(ssize_t start, ssize_t wantedSize);
        int packetsReady();
        void receivePacket(ssize_t* start, ssize_t* datasize, char* data);
    public:
        Transport(std::string ip, int port, std::string fileName, ssize_t size);
        void transportStart();
};