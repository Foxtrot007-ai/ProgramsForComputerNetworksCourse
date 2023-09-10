#include <iostream>
#include "transport.h"

int main(int argc, char **argv){
    if(argc != 5){
        std::cerr << "Incorrect input :( " << std::endl;
        exit(EXIT_FAILURE);
    }
    std::string ip(argv[1]);
    int port = atoi(argv[2]);
    std::string fileName(argv[3]);
    ssize_t dataSize = atol(argv[4]);

    Transport download = Transport(ip, port, fileName, dataSize);
    download.transportStart();
    return 0;
}