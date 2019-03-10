
#define BUF_SIZE 1000

class Packet {
private:
    unsigned int length;
    unsigned int recv_length;
    char payload[BUF_SIZE];
public:
    Packet(char *buf);
    Packet(int fd, int _length);
    int sendPayload(int fd, struct sockaddr *addr, int len);
    void print();
    unsigned int checksum();
    
    char* getPayload() {return payload;}
    int getLength() {return length;}
};
