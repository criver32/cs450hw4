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

#define WAIT 10

using namespace std;

//map<int, Packet> m;
bool loop;
int sock;

void set_addr(struct sockaddr_in *a, unsigned long addr, unsigned short port) {
	a->sin_addr.s_addr = addr;
	a->sin_port = port;
}

int main(int argc, char** argv) {
	cout << "This is a receiver." << endl;
	
	if(argc<2) {
		cout << "Usage: ./receiver <port>\n";
		exit(1);
	}
	
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

	struct timeval t;
	t.tv_sec = WAIT;
	t.tv_usec = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t)) < 0) {
		perror("setsockopt");
		exit(1);
	}

	struct sockaddr_in addr;	 // internet socket address data structure
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[1])); // byte order is significant
	addr.sin_addr.s_addr = INADDR_ANY;

	struct sockaddr_in remote_addr;	 // internet socket address data structure
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = 0; // byte order is significant
	remote_addr.sin_addr.s_addr = 0;
	//inet_pton(AF_INET,"127.0.0.1",&remote_addr.sin_addr.s_addr);
	
	int res = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
	if(res < 0) {
		perror("Error binding: ");
		exit(1);
	}
	
	unsigned long myaddr = 0;
	unsigned short myport = htons(atoi(argv[1]));
	unsigned long dst_addr = 0;
	unsigned short dst_port = 0;

	char buf[MSS];
	memset(buf,'\0',MSS);
	int current = 1000;
	loop = true;
	bool handshaking = true;
	map<int, Packet> m;
	// Receive data
	while(1) {
		Packet pkt(sock);
		
		if (pkt.getTimeout()) {
			if (loop) continue;

			shutdown(sock,SHUT_RDWR);
			close(sock);
			break;
		}
		else {
			loop = false;
		}

		/* Packet(unsigned long _src_addr, unsigned short _src_port,
			unsigned long _dst_addr, unsigned short _dst_port,
			bool _syn, bool _ack,
			unsigned int _seqno, unsigned int _ackno,
			char *buf) */

		if (pkt.isSyn()) {
			if (dst_port != pkt.getSrcPort() || dst_addr != pkt.getSrcAddr()) {
				handshaking = true;
				dst_port = pkt.getSrcPort();
				dst_addr = pkt.getSrcAddr();
			}
			myaddr = pkt.getDstAddr();
			set_addr(&remote_addr, pkt.getSrcAddr(), pkt.getSrcPort());
			cout << "Got syn." << endl;
			//pkt.print();

			Packet ack(myaddr,myport,dst_port,dst_addr, true, true, current, pkt.getSeqno(), NULL);
			ack.sendPacket(sock, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
			//cout << "Sent ack." << endl;
			//ack.print();
		}
		else if (pkt.check()) {
			if (handshaking && pkt.isAck() == false) {
				cout << "Packet ignored during handshake..." << endl;
				continue;
			}
			
/*
			if (handshaking == false && pkt.isAck()) {
				cout << "Got an ack, but handshake already finished..." << endl;
				continue;
			}
*/
			if (handshaking && pkt.isAck()) {
				//cout << "got ack:" << pkt.getSeqno() << "/" << current << endl;
				if (pkt.getAckno() != current) {
					cout << "Wrong ackno during handshake... " << pkt.getAckno() << "/" << current << endl;
					//pkt.print();
					continue;
				}

				handshaking = false;
			}
			if (pkt.getSeqno() > current + 1) continue;

			if (m.count(pkt.getSeqno()) < 1) { // If we haven't received this packet yet, store it
				m.insert( pair<int, Packet>(pkt.getSeqno(), pkt) );
				current++;
				cout << "***Received packet with seqno " << pkt.getSeqno() << "... ";
			}
			else {
				cout << "Duplicate seqno " << pkt.getSeqno() << "... ";
			}

			// Send an ack
			Packet ack(myaddr,myport,dst_port,dst_addr, false, true, pkt.getSeqno(), pkt.getSeqno(), NULL);
			ack.sendPacket(sock, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
			cout << "Sent ack." << endl;
		}
		else {
			cout << "Checksum failed" << endl;
		}
	}

	// Prepare output file
	FILE *f = fopen("out.txt","w");
	if(!f) {
		perror("problem creating output file");
		exit(1);
	}
	
	// Write to file
	for (map<int, Packet>::iterator iter = m.begin(); iter != m.end(); ++iter) {
		char *p = iter->second.getPayload();
		//iter->second.print();
		//cout << "writing:" << endl << p << endl << endl;
		fprintf(f, "%s", p);
	}
	fclose(f);

	//shutdown(sock,SHUT_RDWR);
	//close(sock);
	
	return 0;
}
