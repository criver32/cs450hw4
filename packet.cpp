//
// packet.cpp
//

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <cstdlib>
#include <iostream>
#include "packet.h"

using namespace std;

Packet::Packet(char *buf) {
    memset(payload, 0, BUF_SIZE);
    strcpy(payload, buf);
    length = strlen(payload) + 1;
    recv_length = length;
}

Packet::Packet(int fd, int _length) {
    length = _length;
    memset(payload, 0, BUF_SIZE);
    
    int recv_count = recv(fd, payload, length, 0);
    if (recv_count != length)
        cout << "Payload bytes received: " << recv_count << "/" << length << endl;
    recv_length = recv_count;
}

int Packet::sendPayload(int fd, struct sockaddr *addr, int len) {
    
    int send_count = sendto(fd, payload, length, 0,addr,len);
    if (send_count != length) {
        cout << "Payload bytes sent: " << send_count << "/" << length << endl;
        return -1;
    }
    return 0;
}

void Packet::print() {cout << payload << endl;}

unsigned int Packet::checksum() {
    // TODO
    if (recv_length != length)
        return -1;
    
    unsigned int result = 7;
    int i;
    for (i = 0; i < BUF_SIZE; i++) {
        if (payload[i] == '\0') break;
        result = result*31 + (unsigned int)payload[i];
    }
    return result;
}
