//Dominik Baziuk 322735
#include "webserver.h"

int main(int argc, char **argv){
    if(argc != 3){
        ERROR("Incorrect input");
    }
    int port = atoi(argv[1]);
    std::string directory(argv[2]);
    WebServer webserver = WebServer(port, directory);
    webserver.run();

    return 0;
}
