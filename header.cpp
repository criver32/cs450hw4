#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <cstdlib>
#include <iostream>
#include "header.h"

#define BUF_SIZE 500

using namespace std;

Header::Header(unsigned int _len, bool _syn, bool _ack,
               unsigned int _seqno, unsigned int _ackno, unsigned int _chk) {
    length = _len;
    syn = _syn;
    ack = _ack;
    seqno = _seqno;
    ackno = _ackno;
    checksum = _chk;
}

Header::Header(int fd) {
    seqno = 0;
    ackno = 0;
    ack = false;
    syn = false;
    length = 0;
    checksum = 0;
    
    int recv_count = recv(fd, &length, sizeof(length), 0);
    if (recv_count != sizeof(length))
        cout << "Length bytes received: " << recv_count << "/" << sizeof(length) << endl;
    recv_count = recv(fd, &syn, sizeof(syn), 0);
    if (recv_count != sizeof(syn))
        cout << "Syn bytes received: " << recv_count << "/" << sizeof(syn) << endl;
    recv_count = recv(fd, &ack, sizeof(ack), 0);
    if (recv_count != sizeof(ack))
        cout << "Ack bytes received: " << recv_count << "/" << sizeof(ack) << endl;
    recv_count = recv(fd, &seqno, sizeof(seqno), 0);
    if (recv_count != sizeof(seqno))
        cout << "Seqno bytes received: " << recv_count << "/" << sizeof(seqno) << endl;
    recv_count = recv(fd, &ackno, sizeof(ackno), 0);
    if (recv_count != sizeof(ackno))
        cout << "Ackno bytes received: " << recv_count << "/" << sizeof(ackno) << endl;
    recv_count = recv(fd, &checksum, sizeof(checksum), 0);
    if (recv_count != sizeof(checksum))
        cout << "Checksum bytes received: " << recv_count << "/" << sizeof(checksum) << endl;
}

int Header::sendHeader(int fd, struct sockaddr *addr, int len) {
    // TODO
    
    int send_count = sendto(fd, &length, sizeof(length), 0,addr,len);
    if (send_count != sizeof(length)) {
        cout << "Length bytes sent: " << send_count << "/" << sizeof(length) << endl;
    }
    send_count = sendto(fd, &syn, sizeof(syn), 0,addr,len);
    if (send_count != sizeof(syn)) {
        cout << "Syn bytes sent: " << send_count << "/" << sizeof(syn) << endl;
    }
    send_count = sendto(fd, &ack, sizeof(ack), 0,addr,len);
    if (send_count != sizeof(ack)) {
        cout << "Ack bytes sent: " << send_count << "/" << sizeof(ack) << endl;
    }
    send_count = sendto(fd, &seqno, sizeof(seqno), 0,addr,len);
    if (send_count != sizeof(seqno)) {
        cout << "Seqno bytes sent: " << send_count << "/" << sizeof(seqno) << endl;
    }
    send_count = sendto(fd, &ackno, sizeof(ackno), 0,addr,len);
    if (send_count != sizeof(ackno)) {
        cout << "Ackno bytes sent: " << send_count << "/" << sizeof(ackno) << endl;
    }
    send_count = sendto(fd, &checksum, sizeof(checksum), 0,addr,len);
    if (send_count != sizeof(checksum)) {
        cout << "Checksum bytes sent: " << send_count << "/" << sizeof(checksum) << endl;
    }
    
    return 0;
}

void Header::print() {
    cout << "------------------------------" << endl
    << "Header info:" << endl
    << "Length:\t" << length << endl;
    
    if (syn) { cout << "Syn:\tTRUE" << endl; }
    else { cout << "Syn:\tFALSE" << endl; }
    
    if (ack) { cout << "Ack:\tTRUE" << endl; }
    else { cout << "Ack:\tFALSE" << endl; }
    
    cout << "Seqno:\t" << seqno << endl
    << "Ackno:\t" << ackno << endl
    << "Chksum:\t" << checksum << endl
    << "------------------------------" << endl;
}
