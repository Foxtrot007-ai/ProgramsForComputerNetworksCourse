#pragma once
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <regex>
#include <sys/stat.h>

#define ERROR(str) { printf("%s: %s\n", str, strerror(errno)); exit(EXIT_FAILURE); }
#define waitingtime 1
#define BUFFER_SIZE 1000000

class WebServer{
    private:
        int sockfd;
        struct sockaddr_in server_address;
        int port;
        std::string directory;
        u_int8_t recv_buffer[BUFFER_SIZE+1];
        //read_http_request
        std::string get;
        std::string host;
        std::string connection;
        //validate_path
        std::string true_path;
        std::string file_type;
    public:
        WebServer(int port, std::string directory);
        ssize_t recv_select(int fd);
        bool read_http_request();
        int validate_path();
        std::string make_http_response(int code);
        void send_response(int fd, u_int8_t* buffer, size_t n);
        void run();
        
};