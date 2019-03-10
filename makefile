BINS=sender receiver
all: $(BINS)

receiver: receiver.cpp packet.o header.o
	g++ -o receiver receiver.cpp packet.o header.o

sender: sender.cpp packet.o header.o
	g++ -o sender sender.cpp packet.o header.o

packet.o: packet.cpp packet.h
	g++ -c packet.cpp

header.o: header.cpp header.h
	g++ -c header.cpp

clean:
	rm -f *.o sender receiver