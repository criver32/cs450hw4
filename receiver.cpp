//
// Receiver
//

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <signal.h>

#include <iostream>
#include <map>
#include <iterator>

#include "header.h"
#include "packet.h"

using namespace std;

FILE *f;

void handle_alarm(int sig) {
    fprintf(stderr,"\nMore than a second passed after the last packet. Exiting.\n");
    fclose(f);
    exit(1);
}

int main(int argc, char** argv) {
    cout << "This is a receiver." << endl;
    
    f = NULL;
    
    if(argc<2) {
        cout << "Usage: ./receiver <port>\n";
        exit(1);
    }
    signal(SIGALRM,handle_alarm);
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) {
        perror("Creating socket failed: ");
        exit(1);
    }
    
    struct sockaddr_in addr;     // internet socket address data structure
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1])); // byte order is significant
    addr.sin_addr.s_addr = INADDR_ANY;
    
    int res = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    if(res < 0) {
        perror("Error binding: ");
        exit(1);
    }
    
    // Prepare output file
    f = fopen("out.txt","w");
    if(!f) {
        perror("problem opening file");
        exit(0);
    }
    
    char buf[BUF_SIZE];
    memset(buf,'\0',BUF_SIZE);
    map<int, Packet> m;
    
    // Receive data and write to file
    while(1) {
        Header hdr(sock);
        Packet pkt(sock, hdr.getLength());
        
        if (pkt.checksum() == hdr.getChecksum()) {
            char *p = pkt.getPayload();
            fprintf(f, "%s", p);
            cout << "Received packet with seqno " << hdr.getSeqno() << endl;
        }
        else {
            cout << "Checksum did not match for seqno " << hdr.getSeqno() << endl;
        }
        
        // stop receiving after a second has passed
        alarm(1);
        
        //if(recv_count<0) { perror("Receive failed");    exit(1); }
        //write(1,buf,recv_count);
    }
    
    shutdown(sock,SHUT_RDWR);
    close(sock);
    
    return 0;
}
