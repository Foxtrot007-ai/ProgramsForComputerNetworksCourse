//Dominik Baziuk 322735
#include "webserver.h"

WebServer::WebServer(int port, std::string directory){
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
        ERROR("socket error");
    }

    if(port <= 0 || port > 65535){
        ERROR("port error");
    }

    struct stat sb;
    if (stat(directory.c_str(), &sb) != 0){
        ERROR("directory does not exist error");
    }

    if(!(sb.st_mode & S_IFDIR)){
        ERROR("directory is not directory error");
    }
    
    this->port = port;
    this->directory = directory;

    bzero (&server_address, sizeof(server_address));
    server_address.sin_family      = AF_INET;
    server_address.sin_port        = htons(port);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind (sockfd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0){
        ERROR("bind error");
    }
    if (listen (sockfd, 64) < 0){
        ERROR("listen error");
    }
}

ssize_t WebServer::recv_select(int fd)
{
    struct timeval tv; 
    tv.tv_sec = waitingtime; 
    tv.tv_usec = 0;
    fd_set descriptors;
    FD_ZERO(&descriptors);
    FD_SET(fd, &descriptors);
    int ready = select(fd + 1, &descriptors, NULL, NULL, &tv);
    //std::cout << "ready: " << ready << std::endl;
    if (ready < 0){
        ERROR("select error");
    }
    if (ready == 0){
        return -2;
    }
    ssize_t bytes_read = recv(fd, recv_buffer, BUFFER_SIZE, 0);
    //printf("Bytes received: %ld\n", bytes_read);
    if (bytes_read < 0){
        ERROR("recv error");
    }

    return bytes_read;
   
}

bool WebServer::read_http_request(){
    std::string http_request((char*)recv_buffer);
    //std::cout << "################################" << std::endl;
    //std::cout << http_request << std::endl;
    //std::cout << "################################" << std::endl;
    std::regex regex_get("^GET (\\/.*) HTTP\\/1\\.1\\r\\n");
    std::regex regex_host("Host: (.*)\\r\\n");
    std::regex regex_connection("Connection: (.*)\\r\\n");
    std::smatch mg;
    std::smatch mh;
    std::smatch mc;
    
    if(std::regex_search(http_request, mg, regex_get)){
        get = mg[1];
    }else{
        return false;
    }

    if(std::regex_search(http_request, mh, regex_host)){
        host = mh[1];
    }else{
        return false;
    }

    if(std::regex_search(http_request, mc, regex_connection)){
        connection = mc[1];
    }else{
        return false;
    }
    //std::cout << get << std::endl;
    //std::cout << host << std::endl;
    //std::cout << connection << std::endl;
    return true;
}

int WebServer::validate_path(){
    std::string domain = host.substr(0, host.find_last_of(":"));
    true_path = directory + "/" + domain;
    if(get != "/"){
        true_path += get;
    }
    //std::cout << host << std::endl;
    //std::cout << true_path << std::endl;
    struct stat sb;
    if (stat(true_path.c_str(), &sb) != 0){
        return 404;
    }
    
    if (true_path.find("/..") != std::string::npos){
        return 403;
    }

    if(sb.st_mode & S_IFDIR){
        return 301;
    }

    std::string type = true_path.substr(true_path.find_last_of(".") + 1);
    if(type == "txt") {
        file_type = "text/plain; charset=utf-8";
    } else if(type == "html"){
        file_type = "text/html; charset=utf-8";
    } else if(type == "css"){
        file_type = "text/css; charset=utf-8";
    } else if(type == "jpg"){
        file_type = "image/jpeg";
    } else if(type == "jpeg"){
        file_type = "image/jpeg";
    } else if(type == "png"){
        file_type = "image/png";
    } else if(type == "pdf"){
        file_type = "application/pdf";
    } else{
        file_type = "application/octet-stream";
    }
    return 200;
}

std::string WebServer::make_http_response(int code){
    std::string response = "";
    std::ifstream file(true_path);
    std::stringstream buffer;
    switch(code){
        case 200:
            response = "HTTP/1.1 200 OK\r\nContent-Type: " + file_type + "\r\nContent-Length: ";
            buffer << file.rdbuf();
            response += std::to_string(buffer.str().size()) + "\r\n\r\n" + buffer.str();
        break;
        case 301:
            response =
"HTTP/1.1 301 Moved Permanently\r\n\
Location: http://" + host + "/index.html\r\n\
Content-Type: text/html; charset=utf-8\r\n\
Content-Length: 175\r\n\
Connection: keep-alive\r\n\r\n\
<html>\n\
<head><title>301 Moved Permanently</title></head>\n\
<body>\n\
<center><h1>301 Moved Permanently: You probably wanted to access the directory </h1></center>\n\
</body>\n\
</html>\n";
        break;
        case 403:
            response =
"HTTP/1.1 403 Forbidden\r\n\
Content-Type: text/html; charset=utf-8\r\n\
Content-Length: 148\r\n\
Connection: keep-alive\r\n\r\n\
<html>\n\
<head><title>403 Forbidden</title></head>\n\
<body>\n\
<center><h1>403 Forbidden: File outside the domain directory</h1></center>\n\
</body>\n\
</html>\n";
        break;
        case 404:
            response =
"HTTP/1.1 404 Not Found\r\n\
Content-Type: text/html; charset=utf-8\r\n\
Content-Length: 143\r\n\
Connection: keep-alive\r\n\r\n\
<html>\n\
<head><title>404 Not Found</title></head>\n\
<body>\n\
<center><h1>404 Not Found: File probably does not exist</h1></center>\n\
</body>\n\
</html>\n";
        break;
        case 501:
            response =
"HTTP/1.1 501 Not Implemented\r\n\
Content-Type: text/html; charset=utf-8\r\n\
Content-Length: 151\r\n\
Connection: keep-alive\r\n\r\n\
<html>\n\
<head><title>Function Not Implemented</title></head>\n\
<body>\n\
<center><h1>501 Not Implemented: Can't parse request</h1></center>\n\
</body>\n\
</html>\n";
        break;
        default:
            ERROR("make response");
        break;
    }
    //std::cout << response << std::endl;
    return response;
}

void WebServer::send_response(int fd, u_int8_t* buffer, size_t n)
{
    size_t n_left = n;
    while (n_left > 0) {
        ssize_t bytes_sent = send(fd, buffer, n_left, 0);
        if (bytes_sent < 0){
            ERROR("send error");
        }
        //printf("%ld bytes sent\n", bytes_sent);
        n_left -= bytes_sent;
        buffer += bytes_sent;
    }
}

void WebServer::run(){
    for (;;) {
        //printf("Waiting for connection\n");
        int connected_sockfd = accept(sockfd, NULL, NULL);
        if (connected_sockfd < 0){
            ERROR("accept error");
        }

        //printf("Connection Started\n");
        for(;;){
            ssize_t bytes_read = recv_select(connected_sockfd);
            if (bytes_read == -2) {
                if (close(connected_sockfd) < 0){
                    ERROR("close error");
                }
                //printf("Timeout\n");
                break;
            } if (bytes_read == 0) {
                //printf ("Client closed connection");
                if (close(connected_sockfd) < 0){
                    ERROR("close error");
                }
                break;
            } else {
                std::string message;
                bool parsing_success = read_http_request();
                if(parsing_success){
                    int code = validate_path();
                    message = make_http_response(code);
                }else{
                    connection = "";
                    message = make_http_response(501);
                }
                send_response(connected_sockfd, (u_int8_t*)message.c_str(), message.size());
                if(connection == "close"){
                    if(close (connected_sockfd) < 0){
                        ERROR("close error");
                    }
                    //printf("Closing connection\n");
                    break;
                }
            }
        }
    }
}
