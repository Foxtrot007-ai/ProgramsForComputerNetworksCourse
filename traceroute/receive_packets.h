#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>


bool check_packet(int ttl, uint8_t *buffer, bool &destination);
void receive_packets(int sockfd, int ttl, int &avg, struct timeval &start, bool &destination_reached, std::string *received_ips, int &received_packets);
void print_output(int ttl, std::string *received_ips, int received_packets, int avg);