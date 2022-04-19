all: sender receiver

swpsender.o: swpsender.cpp
	g++ -c swpsender.cpp -o swpsender.o -Wall -Werror

swpreceiver.o: swpreceiver.cpp
	g++ -c swpreceiver.cpp -o swpreceiver.o -Wall -Werror

sender.o: sender.cpp
	g++ -c sender.cpp -o sender.o -Wall -Werror

receiver.o: receiver.cpp
	g++ -c receiver.cpp -o receiver.o -Wall -Werror

sender: swpsender.o sender.o
	g++ -o sender swpsender.o sender.o -Wall -Werror

receiver: swpreceiver.o receiver.o
	g++ -o receiver swpreceiver.o receiver.o -Wall -Werror

clean:
	rm -f *.o sender receiver test.txt


