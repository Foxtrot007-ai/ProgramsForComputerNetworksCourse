#include "transport.h"

void Transport::validateInput(){
    //ip check
    struct sockaddr_in temp;
    if(inet_pton(AF_INET,  ip.c_str(), &(temp.sin_addr)) == -1){
        std::cerr << "Incorrect IP.\n";
        exit(EXIT_FAILURE);
    }
    //port check
    if(port <= 0 || port > 65535){
        std::cerr << "Incorrect port.\n";
        exit(EXIT_FAILURE);
    }
    //size check
    if(size < 0){
        std::cerr << "Incorrect size.\n";
        exit(EXIT_FAILURE);
    }
}

void Transport::init(){
    //init socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1){
        std::cerr << "Failed to create socket.\n";
        exit(EXIT_FAILURE);
    }

    //init file
    filefd = open(fileName.c_str(), O_CREAT | O_WRONLY, 0600);
    if(filefd == -1){
        std::cerr << "Failed to create output file.\n";
        exit(EXIT_FAILURE);
    }

    //create receiver
    bzero(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serverAddress.sin_addr);
 
}

void Transport::sendPacket(ssize_t start, ssize_t wantedSize){
    std::string message = "GET " + std::to_string(start) + " " + std::to_string(wantedSize) + "\n";
    ssize_t messageLenght = strlen(message.c_str());
    ssize_t bytesSent = sendto(sockfd, message.c_str(), messageLenght, 0, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    if(bytesSent != messageLenght){
        std::cerr << strerror(errno) << std::endl;
        std::cerr << "Failed to send message.\n";
        exit(EXIT_FAILURE);
    }
}

int Transport::packetsReady(){
    fd_set descriptors;
    FD_ZERO (&descriptors);
    FD_SET (sockfd, &descriptors);
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;

    int ready = select(sockfd+1, &descriptors, NULL, NULL, &tv);

    if(ready == -1){
        std::cerr << "Failed to wait?(select).\n";
        exit(EXIT_FAILURE);
    }

    return ready;
}

void Transport::receivePacket(ssize_t* start, ssize_t* datasize, char* buffer){
    struct sockaddr_in sender;
    socklen_t senderLen = sizeof(sender);
    ssize_t bytesReceived=recvfrom(sockfd, buffer, UINT16_MAX, 0, (struct sockaddr*) &sender, &senderLen);
    if(bytesReceived == -1){
        std::cerr << "Failed to receive packet.\n";
        exit(EXIT_FAILURE);
    }

    if(sender.sin_addr.s_addr != serverAddress.sin_addr.s_addr || sender.sin_port != serverAddress.sin_port){
        *start = -1;
        *datasize = -1;
        return;
    }
}

Transport::Transport(std::string ip, int port, std::string fileName, ssize_t size){
    this->ip = ip;
    this->port = port;
    this->fileName = fileName;
    this->size = size;
    validateInput();
    init();
    bytesReceived = 0;
}

void Transport::transportStart(){
    //window initiation
    for(int i = 0; i < windowSize; i++){
        window.push_back({-1, new char[packetSize + 1]});
    }

    while(bytesReceived != size){
        //packet request sending

        ssize_t bytesRemaining = size - bytesReceived;
        ssize_t packetRequest = bytesRemaining / packetSize;
        packetRequest = (packetRequest > windowSize) ? windowSize : packetRequest;

        if(packetRequest != 0){
            for(int i = 0; i < packetRequest; i++){
                if(window[i].first == -1){
                    sendPacket(bytesReceived + i * packetSize, packetSize);
                }
            }
        }else{
            sendPacket(size - size % packetSize, size % packetSize);
        }

        //reading packets
        int packetsToRead = packetsReady();
        for(int i = 0; i < packetsToRead; i++){
            char data[IP_MAXPACKET + 1];
            ssize_t start = 0;
            ssize_t datasize = 0;

            receivePacket(&start, &datasize, data);
           
            
            //check if data is from sender
            if(start == -1 || datasize == -1){
                continue;
            }
            
            char dataStr[24];
            int test1 = sscanf(data, "%s %ld %ld\n", dataStr, &start, &datasize);
            if(test1 == 0 || test1 == EOF){
                continue;
            }
            std::string test2(dataStr);
            if(test2 != "DATA"){
                continue;
            }

            int index = (start - bytesReceived) / packetSize;
            //check if received before window moved
            if(index < 0){
                continue;
            }

            //check if received earlier
            if(window[index].first != -1){
                continue;
            }

             //finally put data in window
            window[index].first = datasize;
            int count = 0;
            while(data[count] != '\n') count++;
            window[index].first = datasize;
            memmove(window[index].second,  data + count + 1, datasize);
        }

        //saving data
        while(window[0].first != -1){
            ssize_t dataToWriteLen = window[0].first;
            char* dataToWrite = window[0].second;
            ssize_t bytesWritten = write(filefd, dataToWrite, dataToWriteLen);
            if(bytesWritten < 0){
                std::cerr << "Failed to write to file.\n";
                exit(EXIT_FAILURE);
            }
            bytesReceived += bytesWritten;
            char* usedBuffor = window[0].second;
            window.pop_front();
            window.push_back({-1, usedBuffor});
            //std::cout << (bytesReceived * 100) / size << "%" << std::endl;
        }
    }

    //ending
    for(int i = 0; i < windowSize; i++){
        if(window[i].first != -1){
            delete[] window[i].second;
        }
    }
    close(sockfd);
    close(filefd);
}