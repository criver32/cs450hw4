//
// header.h
//

class Header {
private:
    unsigned int seqno;
    unsigned int ackno;
    bool ack;
    bool syn;
    unsigned int length;
    unsigned int checksum;
public:
    Header(unsigned int _len, bool _syn, bool _ack,
           unsigned int _seqno, unsigned int _ackno, unsigned int _chk);
    Header(int fd);
    
    int sendHeader(int fd, struct sockaddr *addr, int len);
    void print();
    
    unsigned int getSeqno() {return seqno;}
    int getAckno() {return ackno;}
    unsigned int isAck() {return ack;}
    bool isSyn() {return syn;}
    unsigned int getLength() {return length;}
    unsigned int getChecksum() {return checksum;}
};
