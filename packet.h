
#define MSS 1500
#define HEADER_SIZE 50

class Packet {
private:
	unsigned long src_addr;
	unsigned short src_port;
	unsigned long dst_addr;
	unsigned short dst_port;

	bool syn;
	bool ack;
	bool fin;

	unsigned int seqno;
	unsigned int ackno;

	unsigned int length;
	unsigned int checksum;
	unsigned int recv_length;

	char payload[MSS - HEADER_SIZE];
	char buffer[MSS];

	bool timeout;

	int loadHeader();
	int unloadHeader();

	unsigned int computeChecksum(char *buf, int len);
public:
	Packet(unsigned long _src_addr, unsigned short _src_port,
		unsigned long _dst_addr, unsigned short _dst_port,
		bool _syn, bool _ack,
		unsigned int _seqno, unsigned int _ackno,
		char *buf, unsigned int _len);

	Packet(int fd);
	int sendPacket(int fd, struct sockaddr *addr, int len);
	void print();
	bool check();

	void setFin(bool f) { fin = f; }
	void setSeqno(unsigned int s) { seqno = s; }
	void setAckno(unsigned int a) { ackno = a; }

	unsigned long getSrcAddr() { return src_addr; }
	unsigned short getSrcPort() { return src_port; }
	unsigned long getDstAddr() { return dst_addr; }
	unsigned short getDstPort() { return dst_port; }
	bool isSyn() { return syn; }
	bool isAck() { return ack; }
	bool isFin() { return fin; }
	unsigned int getSeqno() { return seqno; }
	unsigned int getAckno() { return ackno; }
	unsigned int getLength() { return length; }
	char* getPayload() { return payload; }
	bool getTimeout() { return timeout; }

};
