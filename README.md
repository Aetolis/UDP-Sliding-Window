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
The SWPSender class is defined in the file `swpsender.cpp` and implements functionality for the sender side of the Sliding Window Protocol. 

## SWPReceiver
The SWPReceiver class is defined in the file `swpreceiver.cpp` and implements functionality for the receiver side of the Sliding Window Protocol.


