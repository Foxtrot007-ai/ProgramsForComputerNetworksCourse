//Dominik Baziuk 322735
#include "receive_packets.h"

void print_output(int ttl, std::string *received_ips, int received_packets, int avg){
    std::cout << ttl << ". ";
    if(received_packets != 0){
        if(received_ips[0] != "")
            std::cout << received_ips[0] << " ";
    
        if(received_ips[1] != received_ips[0] && received_ips[1] != "")
            std::cout << received_ips[0] << " ";
    
        if(received_ips[2] != received_ips[1] && received_ips[2] != received_ips[0] && received_ips[2] != "")
            std::cout << received_ips[0] << " ";
        
        if(received_packets == 3)
            std::cout << avg / 3000 << "ms\n";
        else
            std::cout << "???\n";
    }else{
        std::cout << "*\n";
    }
}

bool check_packet(int ttl, uint8_t *buffer, bool &destination){
    struct ip* ip_header = (struct ip*) buffer;
    ssize_t ip_header_len = 4 * ip_header->ip_hl;
    struct icmp* icmp_header = (struct icmp*) (buffer + ip_header_len);

    uint8_t type = icmp_header->icmp_type;

    if(type == ICMP_TIME_EXCEEDED){
        ip_header = (struct ip *) ((uint8_t *)icmp_header + 8);
        icmp_header = (struct icmp *) ((uint8_t*) ip_header + 4 * ip_header->ip_hl);
    }

    if(type == ICMP_TIME_EXCEEDED || type == ICMP_ECHOREPLY){
        int16_t received_seq = icmp_header->icmp_hun.ih_idseq.icd_seq;
        if(icmp_header->icmp_hun.ih_idseq.icd_id == getpid()
        && (received_seq == 3 * ttl || received_seq == 3 * ttl - 1  || received_seq == 3 * ttl - 2)){ //checking sequence 
           if(type == ICMP_ECHOREPLY) destination = true;
           return true;
        }
    }

    return false;
}

void receive_packets(int sockfd, int ttl, int &avg, struct timeval &start, bool &destination_reached, std::string *received_ips, int &received_packets){
    fd_set descriptors;
    FD_ZERO (&descriptors);
    FD_SET (sockfd, &descriptors);
    struct timeval tv, curr, temp_time;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    while(true){
        //wait 1 sec for data in socket
        int ready = select(sockfd+1, &descriptors, NULL, NULL, &tv);
        
        if(ready == -1){
            std::cout << "Failed to wait?(select).\n";
            exit(EXIT_FAILURE);
        }

        if(ready == 0 || received_packets == 3){
            break;
        }
        
        struct sockaddr_in sender;
        socklen_t sender_len = sizeof(sender);
        uint8_t buffer[IP_MAXPACKET];
        
        //receive data from socket
        ssize_t packet_len = recvfrom(
            sockfd,
            buffer,
            IP_MAXPACKET,
            0,
            (struct sockaddr*)&sender,
            &sender_len
        );

        if(packet_len == -1){
            std::cout << "Failed to receive.\n";
            exit(EXIT_FAILURE);
        }

        //translate IP
        char ip_str[20];
        if(inet_ntop(AF_INET, &(sender.sin_addr), ip_str, sizeof(ip_str)) == NULL){
            std::cout << "IP translation failed.\n";
            exit(EXIT_FAILURE);
        }

        //if we can update info about received packets do it
        if(check_packet(ttl, buffer, destination_reached)){
            received_ips[received_packets] = ip_str;
            gettimeofday(&curr, NULL);
            timersub(&curr, &start, &temp_time);
            avg += temp_time.tv_sec * 1000000 + temp_time.tv_usec;
            received_packets++;
        }
    }
}