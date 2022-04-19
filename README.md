# UDP-Sliding-Window
This repository implements the Sliding Window Protocol as a reliable message passing protocol on top of UDP/IP.

## Makefile
Use `make` or `make all` to create the executables `./sender` and `./receiver` . The `./sender <hostname> <filename>` requires the user to specify a hostname and filename (e.g. `./sender localhost "A Modest Proposal.txt"`), while on the other hand the `./receiver` executable does not take any arguments.

## swp.h
The file `swp.h` is a shared header file that contains include directives, pre-defined constants, in addition to the class declarations for `SWPReceiver` and `SWPSender`. 

We also define two structs: `Packet` and `RecvPacket` that we use to keep track of each packet in our buffer for the sender and receiver respectively.

```c++
typedef struct Packet {
    uint32_t seq_num;
    uint16_t data_len;
    char packet[MAX_PACKET_SIZE];
    clock_t timestamp;
} Packet;
```
For the sender, the struct `Packet` keeps track of `seq_num`, the sequence number of the packet, `data_len`, the length of the data field in the packet, `packet`, the buffer containing our packet, and `timestamp` which we use to keep track of the ACK timeout.

```c++
typedef struct RecvPacket {
    uint32_t seq_num;
    uint16_t data_len;
    char data[MAX_DATA_SIZE];
} RecvPacket;
```
Similarily, for the receiver, we use the struct `RecvPacket` to keep track of the data that the receiver receives. The `seq_num` like before keeps track of the sequence number of the corresponding packet, `data_len` the length of the data that is being read in from the packet, and `data` the buffer that stores the data that we read in from the packet.

Furthermore, in `swp.h` we also define an inline function `error(int chance)` that we later use to simulate packet drop on a lossy network.

```c++
// Produce an error with probability 1/chance
inline bool error(int chance)
{
    return (rand() < (RAND_MAX / chance));
}
```

## SWPSender
The `SWPSender` class is defined in the file `swpsender.cpp` and implements functionality for the sender side of the Sliding Window Protocol. The `SWPSender` contains a total of 3 member functions: `connect(char *hostname)`, `send_file(char *filename)`, and `disconnect(uint32_t final_seq_num)`.

Firstly, we define a default constructor for the `SWPSender` class that initializes the required UDP variables and our buffer for the sender side of the protocol.

```c++
// Constructor
SWPSender::SWPSender()
{
    // Initialize UDP variables
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    packet_num = 0;
    LAR = INIT_SEQ_NUM;
    LFS = (INIT_SEQ_NUM + WINDOW_SIZE - 1) % MAX_SEQ_NUM;
    // windowSize = WINDOW_SIZE;

    // Initialize the send buffer
    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        // send_buf[i].ack_status = false;
        send_buf[i].data_len = 0;
        send_buf[i].timestamp = 0;
    }
}
```

Next, we define a simple destructor that closes the socket that we create in the `connect()` method.

```c++
// Destructor
SWPSender::~SWPSender()
{
    // Close socket
    close(sock_fd);
}
```

The first member method that we define is the `connect(char *hostname)` method. The method first connects to the hostname specified as a input parameter to the method and to the `UDP_PORT` pre-defined in the `swp.h` header file. We then send a virtual connection packet to the receiver for `MAX_RETRY` times, using `poll()` to wait for a response. Upon success, the function returns 0 and prints a confirmation message to stdout.

The second and most important method of the `SWPSender` class is `send_file(char *filename)`. We first open the file specified by the input parameter `filename`. Again we use `poll()` to allow us to send data packets and receive ACKs at the same time. We use a while loop that continues to run until the `EOT_flag` variable is true. For each ACK we receive the validate all fields and ensure that it is valid before updating `LAR` and `LFS` accordingly. As for data packets, we populate our window with packets where ever there is space until we reach the end of the file we are sending. We do not immediately stop after reaching the end of the file because it is possible that there are still outstanding packets in our buffer. Each time we send of a data packet, we also keep track of the current time. On the next iteration if the current time exceeds the timeout time, we resend the packet.

## SWPReceiver
The `SWPReceiver` class is defined in the file `swpreceiver.cpp` and implements functionality for the receiver side of the Sliding Window Protocol. 


