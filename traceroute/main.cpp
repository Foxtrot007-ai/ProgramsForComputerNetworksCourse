//Dominik Baziuk 322735
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "send_packet.h"
#include "receive_packets.h"


int main(int argc, char **argv){

    //argc check
    if (argc != 2){
        std::cout << "Incorrect arguments.\n";
        exit(EXIT_FAILURE);
    }

    //ip check
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, argv[1], &(sa.sin_addr));
    if(!result){
        std::cout << "Incorrect IP format.\n";
        exit(EXIT_FAILURE);
    }

    //socket creation
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(sockfd == -1) {
        std::cout << "Failed to create socket.\n";
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in recipient;
    bzero(&recipient, sizeof(recipient));
    recipient.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &recipient.sin_addr);

    bool destination_reached = false;

    //Sending and receiving ICMP packets;
    for(int ttl = 1; ttl <= 30; ttl++){
        struct timeval start;
        gettimeofday(&start, NULL);

        //send three ICMP packets
        for(int i = 0; i < 3; i++)
            send_icmp_packet(sockfd, ttl, i, &recipient);
        
        //try to received something
        std::string received_ips[3] = {"", "", ""};
        int received_packets_number = 0;
        int avg = 0;

        receive_packets(sockfd, ttl, avg, start, destination_reached, received_ips, received_packets_number);

        print_output(ttl, received_ips, received_packets_number, avg);

        if(destination_reached)
            exit(EXIT_SUCCESS);
    }

    return 0;
}