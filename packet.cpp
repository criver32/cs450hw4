//
// packet.cpp
//

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "packet.h"

using namespace std;

Packet::Packet(unsigned long _src_addr, unsigned short _src_port,
		unsigned long _dst_addr, unsigned short _dst_port,
		bool _syn, bool _ack,
		unsigned int _seqno, unsigned int _ackno,
		char *buf) {
	timeout = false;

	memset(buffer, '\0', MSS);
	memset(payload, '\0', MSS - HEADER_SIZE);

	src_addr = _src_addr;
	src_port = _src_port;

	dst_addr = _dst_addr;
	dst_port = _dst_port;
	
	syn = _syn;
	ack = _ack;

	seqno = _seqno;
	ackno = _ackno;
	
	if (buf == NULL || buf[0] == '\0') {
		length = 0;
	}
	else {
		length = strlen(buf) + 1;
		memcpy(payload, buf, MSS - HEADER_SIZE);
	}
	recv_length = length;
	
	checksum = computeChecksum(buf, length);

	int i = loadHeader();
}

Packet::Packet(int fd) {
	memset(buffer, '\0', MSS);
	memset(payload, '\0', MSS - HEADER_SIZE);

	src_addr = 0;
	src_port = 0;

	dst_addr = 0;
	dst_port = 0;
	
	syn = false;
	ack = false;

	seqno = 0;
	ackno = 0;

	length = 0;
	recv_length = 0;
	checksum = 0;
	
	timeout = false;
	int recv_count = recv(fd, buffer, MSS, 0);
	if (recv_count < 1) timeout = true;

	int i = unloadHeader();

	recv_length = strlen(payload) + 1;
	//cout << "~ ackno: " << ackno << endl;
}

int Packet::sendPacket(int fd, struct sockaddr *addr, int len) {
	loadHeader();
	int send_count = sendto(fd, buffer, MSS, 0,addr,len);
	//cout << "$ ackno: " << ackno << endl;
	//cout << "Bytes sent: " << send_count << endl;
	return send_count;
}

void Packet::print() {
	cout << "========================================" << endl;
	cout << "PACKET INFO:" << endl;
	cout << "----------------------------------------" << endl;
	
	char ip[100];
	inet_ntop(AF_INET, &(src_addr), ip, 100);
	cout << "SRC ADDR/PORT:\t" << ip << "/" << ntohs(src_port) << endl;

	inet_ntop(AF_INET, &(dst_addr), ip, 100);
	cout << "DST ADDR/PORT:\t" << ip << "/" << ntohs(dst_port) << endl;

	cout << "----------------------------------------" << endl;
	cout << "SYN:\t\t";
	if (syn) cout << "1" << endl;
	else cout << "0" << endl;
	
	cout << "ACK:\t\t";
	if (ack) cout << "1" << endl;
	else cout << "0" << endl;

	cout << "----------------------------------------" << endl;
	cout << "SEQNO:\t\t" << seqno << endl;
	cout << "ACKNO:\t\t" << ackno << endl;
	cout << "----------------------------------------" << endl;
	cout << "LENGTH:\t\t" << length << endl;
	cout << "CHECKSUM:\t" << checksum << endl;
	cout << "----------------------------------------" << endl;	
	cout << "PAYLOAD:" << endl;
	cout << "----------------------------------------" << endl;
	cout << payload << endl;
	cout << "========================================" << endl;
}

bool Packet::check() {
	//cout << recv_length << "/" << length << endl;
	if (recv_length != length)
		return false;
	return (computeChecksum(payload, length) == checksum);
}

int Packet::loadHeader() {
	memset(buffer, 0, MSS);
	int i = 0;

	memcpy(&(buffer[i]), &src_addr, sizeof(src_addr));
	i += sizeof(src_addr);
	memcpy(&(buffer[i]), &src_port, sizeof(src_port));
	i += sizeof(src_port);

	memcpy(&(buffer[i]), &dst_addr, sizeof(dst_addr));
	i += sizeof(dst_addr);
	memcpy(&(buffer[i]), &dst_port, sizeof(dst_port));
	i += sizeof(dst_port);

	memcpy(&(buffer[i]), &syn, sizeof(syn));
	i += sizeof(syn);
	memcpy(&(buffer[i]), &ack, sizeof(ack));
	i += sizeof(ack);

	memcpy(&(buffer[i]), &seqno, sizeof(seqno));
	i += sizeof(seqno);
	memcpy(&(buffer[i]), &ackno, sizeof(ackno));
	i += sizeof(ackno);
	
	memcpy(&(buffer[i]), &length, sizeof(length));
	i += sizeof(length);
	memcpy(&(buffer[i]), &checksum, sizeof(checksum));
	i += sizeof(checksum);
	
	memcpy(&(buffer[i]), payload, MSS - HEADER_SIZE);

	return i;
}

int Packet::unloadHeader() {
	int i = 0;

	memcpy(&src_addr, &(buffer[i]), sizeof(src_addr));
	i += sizeof(src_addr);
	memcpy(&src_port, &(buffer[i]), sizeof(src_port));
	i += sizeof(src_port);

	memcpy(&dst_addr, &(buffer[i]), sizeof(dst_addr));
	i += sizeof(dst_addr);
	memcpy(&dst_port, &(buffer[i]), sizeof(dst_port));
	i += sizeof(dst_port);

	memcpy(&syn, &(buffer[i]), sizeof(syn));
	i += sizeof(syn);
	memcpy(&ack, &(buffer[i]), sizeof(ack));
	i += sizeof(ack);

	memcpy(&seqno, &(buffer[i]), sizeof(seqno));
	i += sizeof(seqno);
	memcpy(&ackno, &(buffer[i]), sizeof(ackno));
	i += sizeof(ackno);
	
	memcpy(&length, &(buffer[i]), sizeof(length));
	i += sizeof(length);
	memcpy(&checksum, &(buffer[i]), sizeof(checksum));
	i += sizeof(checksum);

	memcpy(payload, &(buffer[i]), MSS - HEADER_SIZE);	
	
	return i;
}

unsigned int Packet::computeChecksum(char *buf, int len) {
	//return 0;
	// TODO
	if (buf == NULL || buf[0] == '\0') return 0;
	unsigned int result = 7;
	int i;
	for (i = 0; i < len; i++) {
		if (buf[i] == '\0') break;
		result = result*31 + (unsigned int)(buf[i]);
	}
	return result;
}


