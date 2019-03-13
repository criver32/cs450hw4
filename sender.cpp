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

#include "packet.h"

#define MIN_DELAY 1000
#define MAX_DELAY 10000000

using namespace std;

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

	int r_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(r_fd < 0) {
		perror("Creating socket failed: ");
		exit(1);
	}

	int yes=1;
 	if (setsockopt(s_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt");
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
	addr.sin_port = htons(8081); // byte order is significant
	addr.sin_addr.s_addr = INADDR_ANY;

	int res = bind(s_fd, (struct sockaddr*)&addr, sizeof(addr));
	if(res < 0) {
		perror("Error binding: ");
		exit(1);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[2])); // byte order is significant
	inet_pton(AF_INET,argv[1],&addr.sin_addr.s_addr);

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
	while(fread(buf,sizeof(char), MSS-HEADER_SIZE, f)) {
		i++;
		Packet pkt(0,0,0,0, false, false, i, 0, buf);
		m.insert( pair<int, Packet>(i, pkt) );
		memset(buf, '\0', MSS);
		m.at(i).print();
	}
	cout << "Total packets stored: " << i << endl;
	int total = i;
	int sent = 0;

	int delay = 10000;

	map<int, Packet>::iterator iter;
	for (iter = m.begin(); iter != m.end(); ++iter) {
		i = iter->first;
		Packet pkt = iter->second;
		
		int x = 0;
		cout << "Sending packet " << i << "..." << endl;
		bool retry = true;
		while (retry) {
			pkt.sendPacket(s_fd, (struct sockaddr*)&addr, sizeof(addr));
			
			cout << x << endl;
			usleep(delay);
			x++;
			cout << "Waiting for ack... ";
			bool timeout = false;
			while (!timeout) {
				Packet ack(s_fd);
				if (ack.isAck() == true && ack.getAckno() == pkt.getSeqno()) {
					retry = false;
					cout << "***Got ack: " << ack.getAckno() << endl;
					//ack.print();

					delay = delay - 20000;
					if (delay < MIN_DELAY) delay = MIN_DELAY;
					printf("delay: %d\n", delay);
					break;
				}
				if (ack.getTimeout()) {
					delay = delay * 1.5;
					if (delay > MAX_DELAY) delay = MAX_DELAY;
					printf("delay: %d\n", delay);
					timeout = true;
					break;
				}
			}
			if (timeout)
				cout << "Retrying..." << endl;
		}
		
		sent++;
	}
	
	cout << sent << "/" << total << " packets sent" << endl;
	
	shutdown(s_fd,SHUT_RDWR);
	close(s_fd);
	
	return 0;
}
