#include <iostream>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <vector>
#include <tuple>
#include <map>
#include <utility>
#include <algorithm>
#include <sys/time.h>
#define MaxDist 20
//                 broadcast/port      dist      ip  
typedef std::tuple<std::string, uint32_t, std::string> row;


class Router{
  private:
    int sockfd;
    std::size_t myNetworksNumber;
    struct sockaddr_in server_address;
    std::vector<row>  myNetworksVector;
    std::vector<row>  distanceVector;
    
    //       cidr         turnsmissed
    std::map<std::string, uint32_t> neighbors;
    void increaseTurn();
    void updateNetworksAvailablility();

    std::pair<std::string, std::string> separateIP(std::string ipWithCidr);
    uint32_t makeNetworkIPBinary(std::string cidr);
    std::string makeNetworkIPString(std::string cidr);
    uint32_t makeBroadcastIPBinary(std::string cidr);
    std::string makeBroadcastIPString(std::string cidr);
    bool isInNetwork(std::string network, std::string ip);
    int makeSocket();
    int sendDistanceVector();
    uint32_t getNetworkDistance(std::string ip);
    void updateRow(std::string senderIp, std::string cidr, uint32_t distance);
  public: 
    Router();
    int runRouter();
    void showRoutingTable();
};