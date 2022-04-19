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

    uint32_t final_seq_num = sender.send_file(argv[2]);

    sender.disconnect(final_seq_num);

    return 0;
}