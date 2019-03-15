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
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <map>
#include <iterator>

#include "packet.h"

#define SEC 1000000

#define MIN_DELAY 100
#define MAX_DELAY SEC * 3
#define PENALTY   SEC / 10
#define MULT	  1.5

using namespace std;

struct timeval timestamp() {
	struct timeval tv;
	struct timezone tz;
	memset(&tv, 0, sizeof(tv));
	memset(&tz, 0, sizeof(tz));
	if (gettimeofday(&tv, &tz) < 0) {
		perror("time:");
	}
	return tv;
}

unsigned long timedif(struct timeval *start, struct timeval *end) {
	return SEC * ( end->tv_sec - start->tv_sec ) + ( end->tv_usec - start->tv_usec );
}

void print_addr(struct sockaddr_in *a) {
	char ip[100];
	inet_ntop(AF_INET, &(a->sin_addr.s_addr), ip, 100);			
	printf("ADDR: %s\nPORT: %d\n", ip, ntohs(a->sin_port));
}

int main(int argc, char** argv) {
	cout << "This is a sender." << endl;
	
	if(argc<4) {
		printf("Usage: ./sender <ip> <port> <filename>\n");
		exit(1);
	}
	int s_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(s_fd < 0) {
		perror("Creating socket failed: ");
		exit(1);
	}

	struct timeval t;
	t.tv_sec = 0;
	t.tv_usec = 100;
	if (setsockopt(s_fd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t)) < 0) {
		perror("setsockopt");
		exit(1);
	}
	
	struct sockaddr_in addr;	 // internet socket address data structure
	addr.sin_family = AF_INET;
	addr.sin_port = 0;
	addr.sin_addr.s_addr = INADDR_ANY;
	
	struct sockaddr_in test_addr;	 // internet socket address data structure
	test_addr.sin_family = AF_INET;
	test_addr.sin_port = 0;
	test_addr.sin_addr.s_addr = INADDR_ANY;

	struct sockaddr_in remote_addr;
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(atoi(argv[2])); // byte order is significant
	inet_pton(AF_INET,argv[1],&remote_addr.sin_addr.s_addr);

	// Get local address
	int test_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(test_sock < 0) {
		perror("Creating socket failed: ");
		exit(1);
	}
	connect(test_sock, (struct sockaddr*)&remote_addr, sizeof(remote_addr));

	socklen_t len = sizeof(test_addr);
	getsockname(test_sock, (struct sockaddr*)&test_addr, &len);
	unsigned long myaddr = test_addr.sin_addr.s_addr;
	char myaddr_buf[100];
	inet_ntop(AF_INET, &(test_addr.sin_addr.s_addr), myaddr_buf, 100);

	shutdown(test_sock,SHUT_RDWR);
	close(test_sock);

	int res = bind(s_fd, (struct sockaddr*)&addr, sizeof(addr));
	if(res < 0) {
		perror("Error binding: ");
		exit(1);
	}

	len = sizeof(addr);
	getsockname(s_fd, (struct sockaddr*)&addr, &len);
	unsigned short myport = addr.sin_port;

	cout << "Using port " << ntohs(myport) << endl;
	cout << "At addr " << myaddr_buf << endl;
	
	/*
	Packet(unsigned long _src_addr, unsigned short _src_port,
		unsigned long _dst_addr, unsigned short _dst_port,
		bool _syn, bool _ack,
		unsigned int _seqno, unsigned int _ackno,
		char *buf)
	*/

	map<int, Packet> m;
	FILE *f = NULL;
	f = fopen(argv[3],"r");
	if(!f) {
		perror("problem opening file");
		exit(0);
	}
	
	// Read in all the data
	char buf[MSS];
	memset(buf, '\0', MSS);
	int i = 0;
	
	m.insert( pair<int, Packet>(0, Packet(myaddr,myport,
						remote_addr.sin_addr.s_addr,
						remote_addr.sin_port,
						true, false, 0, 0, NULL) ) );

	while(fread(buf,sizeof(char), MSS-HEADER_SIZE-1, f)) {
		i++;
		Packet pkt(myaddr,myport,remote_addr.sin_addr.s_addr,remote_addr.sin_port, false, (i == 1), i, 0, buf);
		m.insert( pair<int, Packet>(i, pkt) );
		memset(buf, '\0', MSS);
		//m.at(i).print();
	}
	cout << "Total packets stored: " << i << endl;
	int total = i;
	int sent = 0;

	int delay = 10000;
	int start_seq = 0;
	bool handshaking = true;

	map<int, Packet>::iterator iter;
	for (iter = m.begin(); iter != m.end(); ++iter) {
		Packet pkt = iter->second;
		i = start_seq + pkt.getSeqno();
		pkt.setSeqno(i);
		
		int x = 0;
		cout << "Sending packet " << i << "..." << endl;
		//pkt.print();
		bool retry = true;
		while (retry) {
			pkt.sendPacket(s_fd, (struct sockaddr*)&remote_addr, sizeof(addr));
			
			struct timeval start_t = timestamp();
			cout << "Waiting for ack... ";
			bool timeout = false;
			while (!timeout) {
				Packet ack(s_fd);
				if (ack.isAck() == true && ack.getAckno() == pkt.getSeqno()) {
					retry = false;
					cout << "***Got ack: " << ack.getAckno() << endl;
					if (ack.isSyn() && handshaking) {
						start_seq = ack.getSeqno();
						cout << "Handshake complete: " << start_seq << endl;
						handshaking = false;
						auto next = iter;
						next++;
						next->second.setAckno(ack.getSeqno());
						//next->second.print();
					}
					//ack.print();

					delay = delay - PENALTY;
					if (delay < MIN_DELAY) delay = MIN_DELAY;
					printf("delay: %d\n", delay);
					break;
				}

				struct timeval current_t = timestamp();
				if (delay < timedif(&start_t, &current_t)) {
					delay = delay * MULT;
					if (delay > MAX_DELAY) delay = MAX_DELAY;
					printf("delay: %d\n", delay);
					timeout = true;
					break;
				}
			}
			if (timeout) {
				x++;
				cout << "Retrying... " << x << endl;
			}
		}
		
		sent++;
	}
	
	cout << sent << "/" << total << " packets sent" << endl;
	
	shutdown(s_fd,SHUT_RDWR);
	close(s_fd);
	
	return 0;
}
