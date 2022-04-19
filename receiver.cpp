// receiver.cpp
// Test file for SWPReceiver class

#include "swp.h"

int main (void) {
    SWPReceiver receiver;
    receiver.setup();

    char filename[9] = "test.txt";
    receiver.receive_file(filename);

    return 0;
}