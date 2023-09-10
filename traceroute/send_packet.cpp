//Dominik Baziuk 322735
#include "send_packet.h"

u_int16_t compute_icmp_checksum(const void *buff, int length){
    u_int32_t sum;
    const uint16_t* ptr = (uint16_t*) buff;
    assert(length % 2 == 0);
    for(sum = 0; length > 0; length -= 2)
        sum += *ptr++;
    sum = (sum >> 16) + (sum & 0xffff);
    return (u_int16_t)(~(sum + (sum >> 16)));
}

int send_icmp_packet(int sockfd, int ttl, int seq, sockaddr_in *recipient){
    struct icmp header{};
    header.icmp_type = ICMP_ECHO;
    header.icmp_code = 0;
    header.icmp_hun.ih_idseq.icd_id = getpid();
    header.icmp_hun.ih_idseq.icd_seq = 3 * ttl - seq; // {3 * ttl - 0, 3 * ttl - 1, 3 * ttl - 2} 
    header.icmp_cksum = 0;
    header.icmp_cksum = compute_icmp_checksum((uint16_t*) &header, sizeof(header));

    setsockopt(sockfd,IPPROTO_IP, IP_TTL, &ttl, sizeof(int));

    ssize_t bytes_sent = sendto(
        sockfd,
        &header,
        sizeof(header),
        0,
        (struct sockaddr*) recipient,
        sizeof(*recipient)
    );

    if(bytes_sent == -1){
        std::cout << "Failed to send ICMP.\n";
        exit(EXIT_FAILURE);
    }    

    return bytes_sent;
}