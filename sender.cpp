// sender.cpp
// Test file for SWPSender class

#include "swp.h"

int main (int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <hostname> <filename>\n", argv[0]);
        exit(1);
    }

    SWPSender sender;
    sender.connect(argv[1]);
}