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
#include <arpa/inet.h>
#include <signal.h>

#include <iostream>
#include <map>
#include <iterator>

#include "packet.h"

#define WAIT 15

using namespace std;

//map<int, Packet> m;
bool loop;
int sock;

void handle_alarm(int sig) {

	loop = false;
	shutdown(sock,SHUT_RDWR);
	close(sock);
}

int main(int argc, char** argv) {
	cout << "This is a receiver." << endl;
	
	if(argc<2) {
		cout << "Usage: ./receiver <port>\n";
		exit(1);
	}
	signal(SIGALRM,handle_alarm);
	
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0) {
		perror("Creating socket failed: ");
		exit(1);
	}
	
	int yes=1;
 	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
/*	
	struct timeval t;
	t.tv_sec = 3;
	t.tv_usec = 200;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t)) < 0) {
		perror("setsockopt");
		exit(1);
	}
*/	
	struct sockaddr_in addr;	 // internet socket address data structure
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[1])); // byte order is significant
	addr.sin_addr.s_addr = INADDR_ANY;

	struct sockaddr_in remote_addr;	 // internet socket address data structure
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(8081); // byte order is significant
	inet_pton(AF_INET,"127.0.0.1",&remote_addr.sin_addr.s_addr);
	
	int res = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
	if(res < 0) {
		perror("Error binding: ");
		exit(1);
	}
	
	

	char buf[MSS];
	memset(buf,'\0',MSS);
	int current = 1;
	loop = true;
	map<int, Packet> m;
	// Receive data
	while(1) {
		Packet pkt(sock);
		
		if (loop == false) {
			break;
		}

		/* Packet(unsigned long _src_addr, unsigned short _src_port,
			unsigned long _dst_addr, unsigned short _dst_port,
			bool _syn, bool _ack,
			unsigned int _seqno, unsigned int _ackno,
			char *buf) */

		if (pkt.check()) {
			if (m.count(pkt.getSeqno()) < 1) {
				m.insert( pair<int, Packet>(pkt.getSeqno(), pkt) );
				cout << "***Received packet with seqno " << pkt.getSeqno() << "... ";
			}
			else {
				cout << "Duplicate seqno " << pkt.getSeqno() << "... ";
			}
			Packet ack(0,0,0,0, false, true, pkt.getSeqno(), pkt.getSeqno(), NULL);
			ack.sendPacket(sock, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
			cout << "Sent ack." << endl;
		}
		else {
			cout << "Checksum failed" << endl;
		}
		
		// stop receiving after a second has passed
		alarm(WAIT);
	}	
	
	/*
	for (map<int, Packet>::iterator iter = m.begin(); iter != m.end(); ++iter) {
		char *p = iter->second.getPayload();
		iter->second.print();
		//cout << "writing:" << endl << p << endl << endl;
	}*/

	// Prepare output file
	FILE *f = fopen("out.txt","w");
	if(!f) {
		perror("problem creating output file");
		exit(1);
	}
	
	// Write to file
	for (map<int, Packet>::iterator iter = m.begin(); iter != m.end(); ++iter) {
		char *p = iter->second.getPayload();
		iter->second.print();
		//cout << "writing:" << endl << p << endl << endl;
		fprintf(f, "%s", p);
	}
	fclose(f);

	//shutdown(sock,SHUT_RDWR);
	//close(sock);
	
	return 0;
}
