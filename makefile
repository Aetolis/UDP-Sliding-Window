all: sender receiver

swpsender.o: swpsender.cpp
	g++ -c swpsender.cpp

swpreceiver.o: swpreceiver.cpp
	g++ -c swpreceiver.cpp

sender.o: sender.cpp
	g++ -c sender.cpp

receiver.o: receiver.cpp
	g++ -c receiver.cpp

sender: swpsender.o sender.o
	g++ -o sender swpsender.o sender.o

receiver: swpreceiver.o receiver.o
	g++ -o receiver swpreceiver.o receiver.o

clean:
	rm -f *.o sender receiver


