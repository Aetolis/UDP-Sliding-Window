// receiver.cpp
// Test file for SWPReceiver class

#include "swp.h"

int main (void) {
    SWPReceiver receiver;
    receiver.setup();

    receiver.receive_file("test.txt");
}