//
// Sender
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <map>
#include <iterator>

#include "header.h"
#include "packet.h"

using namespace std;

int main(int argc, char** argv) {
    cout << "This is a sender." << endl;
    
    if(argc<4) {
        printf("Usage: ./sender <ip> <port> <filename>\n");
        exit(1);
    }
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) {
        perror("Creating socket failed: ");
        exit(1);
    }
    
    struct sockaddr_in addr;     // internet socket address data structure
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2])); // byte order is significant
    inet_pton(AF_INET,argv[1],&addr.sin_addr.s_addr);
    
    //Header(_len, _syn, _ack, _seqno, _ackno, _chk)
    
    map<int, Packet> m;
    FILE *f = NULL;
    f = fopen(argv[3],"r");
    if(!f) {
        perror("problem opening file");
        exit(0);
    }
    
    // Read in all the data
    char buf[BUF_SIZE];
    memset(buf, '\0', BUF_SIZE);
    int i = 0;
    while(fgets(buf,BUF_SIZE - 1,f)) {
        i++;
        m.insert( pair<int, Packet>(i, Packet(buf)) );
    }
    cout << "Total packets stored: " << i << endl;
    int total = i;
    int sent = 0;
    
    map<int, Packet>::iterator iter;
    for (iter = m.begin(); iter != m.end(); ++iter) {
        i = iter->first;
        Packet pkt = iter->second;
        
        Header hdr(pkt.getLength(), false, false, i, 0, pkt.checksum());
        cout << "Sending packet " << i << ": header..." << endl;
        int res = hdr.sendHeader(sock, (struct sockaddr*)&addr, sizeof(addr));
        if (res < 0) continue;
        cout << "Sending packet " << i << ": payload..." << endl << endl;
        res = pkt.sendPayload(sock, (struct sockaddr*)&addr, sizeof(addr));
        if (res < 0) continue;
        sent++;
    }
    
    cout << sent << "/" << total << " packets sent" << endl;
    
    shutdown(sock,SHUT_RDWR);
    close(sock);
    
    return 0;
}
