//Dominik Baziuk 322735
#include "router.h"

Router::Router(){
    std::cin >> myNetworksNumber;
    std::string cidr;
    std::string word;
    int dist;
    for(std::size_t i = 0; i < myNetworksNumber; i++){
        std::cin >> cidr >> word >> dist;
        std::pair<std::string, std::string> splitted = separateIP(cidr);
        std::string networkIP = makeNetworkIPString(cidr);
        networkIP += "/" + splitted.second;
        myNetworksVector.push_back(std::make_tuple(networkIP, dist, splitted.first));
        distanceVector.push_back(std::make_tuple(networkIP, dist, splitted.first));
    }
}

int Router::runRouter(){
    if(makeSocket() == -1){
        std::cerr << "Unable to create socket\n";
        return -1;
    }

    while(true){
        fd_set descriptors;
        FD_ZERO (&descriptors);
        FD_SET (sockfd, &descriptors);
        struct timeval tv;
        tv.tv_sec = 15;
        tv.tv_usec = 0;

        updateNetworksAvailablility();
        sendDistanceVector();
        showRoutingTable();
        
        while(true){
            int ready = select(sockfd+1, &descriptors, NULL, NULL, &tv);
            if(ready == -1){
                std::cout << "Failed to wait?(select).\n";
                exit(EXIT_FAILURE);
            }

            if(ready == 0){
                break;
            }

            struct sockaddr_in sender;
            socklen_t sender_len = sizeof(sender);
            char buffer[IP_MAXPACKET+1];
            
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
   
            char senderIpStr[20]; 
            inet_ntop(AF_INET, &(sender.sin_addr), senderIpStr, sizeof(senderIpStr));
            std::string senderIp(senderIpStr);

            uint32_t i,d;
            char p;
            memcpy(&i, buffer, 4);
            memcpy(&p, buffer + 4, 1);
            memcpy(&d, buffer + 5, 4);
            d = ((0xFF000000UL & d) >> 24) |
                ((0x00FF0000UL & d) >> 8) |
                ((0x0000FF00UL & d) << 8)|
                ((0x000000FFUL & d) << 24);

            struct in_addr addr;
            addr.s_addr = i;

            std::string rowCidr(inet_ntoa(addr));
            rowCidr += "/";
            rowCidr += std::to_string(p);

            updateRow(senderIp, rowCidr, d);
        }
        increaseTurn();
    }
    return 0;
}


int Router::sendDistanceVector(){
    for(const auto&  networkInfo : myNetworksVector){
       
        struct sockaddr_in other_address;
	    bzero (&other_address, sizeof(other_address));
	    other_address.sin_family      = AF_INET;
	    other_address.sin_port        = htons(54321);
        //set broadcast ip
        other_address.sin_addr.s_addr = makeBroadcastIPBinary(std::get<0>(networkInfo));

        char ipStr[20]; 
        inet_ntop(AF_INET, &(other_address.sin_addr), ipStr, sizeof(ipStr));

        for(const auto& vectorInfo : distanceVector){
            //dont send info about route if this route via this network
            if(isInNetwork(std::get<0>(networkInfo), std::get<2>(vectorInfo))){
                continue;
            }

            //get informations
            std::pair<std::string, std::string> cidr = separateIP(std::get<0>(vectorInfo));
            const char* data_ip = cidr.first.c_str();
            uint32_t ip = inet_addr(data_ip);
            char data_prefix = std::stoi(cidr.second);
            uint32_t dist = std::get<1>(vectorInfo);
            dist = ((0xFF000000UL & dist) >> 24) |
                ((0x00FF0000UL & dist) >> 8) |
                ((0x0000FF00UL & dist) << 8)|
                ((0x000000FFUL & dist) << 24);

            //make UDP packet to send
            char data[9];
            memcpy(data, &ip, 4);
            memcpy(data + 4, &data_prefix, 1);
            memcpy(data + 5, &dist, 4);

            //and send it
            if(sendto(sockfd, 
                      data, 
                      sizeof(data), 
                      0,
                      (struct sockaddr *) &other_address,
                      sizeof(other_address)) == -1)
            {
                fprintf(stdout, "sendto error, %s\n", strerror(errno));
                for(auto& n : neighbors){
                    if(isInNetwork(std::get<0>(networkInfo), n.first))
                    {
                        neighbors[n.first] += 2;
                    }
                }
            }
        }
    }

    //informations with infinite distance have just sent so we can erase them
    distanceVector.erase(std::remove_if(distanceVector.begin(),
                                        distanceVector.end(),
                                        [&](row r) { return std::get<1>(r) == UINT32_MAX; }),
                        distanceVector.end());
    
    for(auto& network : myNetworksVector){
        if(std::find_if(distanceVector.begin(), distanceVector.end(), 
                        [&] (row r) {return std::get<0>(r) == std::get<0>(network); }) 
           == distanceVector.end()){
           distanceVector.push_back(make_tuple(std::get<0>(network), (neighbors[std::get<2>(network)] == 1) ? std::get<1>(network) : UINT32_MAX, std::get<2>(network)));
        }
    }

    return 0;
}


uint32_t Router::getNetworkDistance(std::string ip){
    for(const auto& neighborInfo : myNetworksVector){
        std::string networkCidr = std::get<0>(neighborInfo);
        if(isInNetwork(networkCidr, ip)){
            neighbors[ip] = 0;
            return std::get<1>(neighborInfo);
        }
    }
        
    return 0;
}

void Router::increaseTurn(){
    for(auto& n : neighbors){
        neighbors[n.first] += 1;
    }
}

void Router::updateNetworksAvailablility(){
    for(auto& vectorInfo : distanceVector){
        if(neighbors[std::get<2>(vectorInfo)] > 2){
            std::get<1>(vectorInfo) = UINT32_MAX;
        }
    }
}

void Router::updateRow(std::string senderIp, std::string cidr, uint32_t distance){
    //get distance from sender, if sender isn't neighbor it returns;
    uint32_t senderDistance = getNetworkDistance(senderIp);
    if(senderDistance == 0){
        return;
    }

    bool found = false;
    for(auto& r : distanceVector){
        if(std::get<0>(r) == cidr){
            //if this network is reachable from sender and now distance is infinite than is unreachable
            if(std::get<2>(r) == senderIp && distance == UINT32_MAX){
                std::get<1>(r) = UINT32_MAX;
                found = true;
                break;
            }
            //check if it is better route
            if(std::get<1>(r) > distance + senderDistance && distance != UINT32_MAX){
                std::get<1>(r) = distance + senderDistance;
                std::get<2>(r) = senderIp; 
            }

            //dist going to infinite
            if(std::get<1>(r) >= MaxDist){
                std::get<1>(r) = UINT32_MAX;
            }

            found = true;                
            break;
        }
    }

    //insert new row into distance vector
    if(!found){
        distanceVector.push_back(std::make_tuple(cidr,(distance == UINT32_MAX) ? distance : distance + senderDistance, senderIp));
    }
}

int Router::makeSocket(){
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		std::cerr << "socket error: " << strerror(errno) << "\n"; 
		return -1;
	}

	bzero (&server_address, sizeof(server_address));
	server_address.sin_family      = AF_INET;
	server_address.sin_port        = htons(54321);
	server_address.sin_addr.s_addr = INADDR_ANY;

	if (bind (sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "bind error:" << strerror(errno) << "\n"; 
		return -1;
	}
    
    int enabled = 1;
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &enabled, sizeof(int)) < 0) {
        std::cerr << "setsockopt error:" << strerror(errno) << "\n"; 
		return -1;
	}
    return 0;
}

bool Router::isInNetwork(std::string network, std::string ip){
    return separateIP(network).first == makeNetworkIPString(ip + "/" + separateIP(network).second);
}


uint32_t Router::makeNetworkIPBinary(std::string cidr){
    struct sockaddr_in other_address;
    bzero (&other_address, sizeof(other_address));

    std::pair<std::string, std::string> splitted = separateIP(cidr);
    const char* other_ip = splitted.first.c_str();
    int prefix = std::stoi(splitted.second);

    inet_pton(AF_INET, other_ip, &other_address.sin_addr);
    
    uint32_t mask = 0xFFFFFFFFUL << (32 - prefix);
    other_address.sin_addr.s_addr &= __builtin_bswap32(mask);

    return  other_address.sin_addr.s_addr;
}


std::string Router::makeNetworkIPString(std::string cidr){
    struct sockaddr_in other_address;
    bzero (&other_address, sizeof(other_address));

    other_address.sin_addr.s_addr = makeNetworkIPBinary(cidr);

    char ipStr[20]; 
    inet_ntop(AF_INET, &(other_address.sin_addr), ipStr, sizeof(ipStr));

    std::string temp(ipStr);
    return temp;
}


uint32_t Router::makeBroadcastIPBinary(std::string cidr){
    struct sockaddr_in other_address;
    bzero (&other_address, sizeof(other_address));

    std::pair<std::string, std::string> splitted = separateIP(cidr);
    const char* other_ip = splitted.first.c_str();
    int prefix = std::stoi(splitted.second);

    inet_pton(AF_INET, other_ip, &other_address.sin_addr);
    
    uint32_t mask = (1 << (32 - prefix)) - 1;
    other_address.sin_addr.s_addr |= __builtin_bswap32(mask);

    return  other_address.sin_addr.s_addr;
}


std::string Router::makeBroadcastIPString(std::string cidr){
    struct sockaddr_in other_address;
    bzero (&other_address, sizeof(other_address));

    other_address.sin_addr.s_addr = makeBroadcastIPBinary(cidr);

    char ipStr[20]; 
    inet_ntop(AF_INET, &(other_address.sin_addr), ipStr, sizeof(ipStr));

    std::string temp(ipStr);
    return temp;
}


void Router::showRoutingTable(){
    std::cout << "\n############################################################\n";
    for(const auto& r : distanceVector){
        std::string cidr = std::get<0>(r);
        std::cout << cidr << " ";

        uint32_t dist = std::get<1>(r);
        std::string via = std::get<2>(r);

        if(dist == UINT32_MAX) std::cout << "unreachable ";
        else std::cout << "distance " << dist << " ";
        
        if(isInNetwork(cidr, via)) std::cout << "connected directly";
        else std::cout << "via " << via;

       
        std::cout << "\n";
    }
    std::cout << "############################################################\n";
}

std::pair<std::string, std::string> Router::separateIP(std::string ipWithCidr){
    std::string ip = "";
    std::string prefix = "";
    bool changed = false;
    for(char c : ipWithCidr){
        if(c == '/'){
            changed = true;
            continue;
        }

        if(!changed)
            ip += c;
        else prefix += c;
    }
    return make_pair(ip, prefix);
}

