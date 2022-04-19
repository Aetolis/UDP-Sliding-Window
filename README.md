# UDP-Sliding-Window
This repository implements the Sliding Window Protocol as a reliable message passing protocol on top of UDP/IP.

## Makefile
Use `make` or `make all` to create the executables `./receiver` and `./sender`. The `./receiver` executable does not take any arguments, while `./sender <hostname> <filename>` requires the user to specify a hostname and filename (e.g. `./sender localhost "A Modest Proposal.txt"`).

## swp.h
The file `swp.h` is a shared header file that contains pre-defined constants in addition to the class declarations for `SWPReceiver` and `SWPSender`. Furthermore, in `swp.h` we also define an inline function `error(int chance)` that we later use to simulate packet drop on a lossy network.

## SWPReceiver

## SWPSender

