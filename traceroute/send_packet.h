#include <iostream>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <assert.h>

u_int16_t compute_icmp_checksum(const void *buff, int length);
int send_icmp_packet(int sockfd, int ttl, int seq, sockaddr_in *recipient);