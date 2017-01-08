# Gameboy Cartridge Dumper - Listener

This is some code to run on a Raspberry Pi to connect to a Gameboy via a 
[link cable breakout board](https://github.com/Palmr/gb-link-cable) and wait for data from the 
[cart-dumper ROM](https://github.com/Palmr/cart-dumper)

I started using the interrupts with WiringPi but found the first few bits would come in out of order
or go missing. I believe the interrupt queue is only 1 ISR so perhaps the Raspberry Pi couldn't handle
the initial bits.

I moved to the regular listner with a manual loop instead and this improved things.

## TODO

- [x] Get GPIO code working on the gameboy
- [x] Add non-blocking ncurses escape to endless loop so file could be written outside
- [ ] Diagnose occasional malformed bytes (timing issue?)
- [ ] Add support for a header-packet with the size of the ROM
- [ ] Parse inbound cart header data to show the ROM details
- [ ] Do checksum in the header to see if we got malformed data
- [ ] Implement some form of data redundancy
  - [ ] Get bytes multiple times, respond with an outbound byte if they all match?
  - [ ] Hamming codes

## To Build

Clone this repository on a Raspberry Pi, run either `gcc listener.c -o listener -lwiringPi -lncurses`
or `gcc listener-interrupts.c -o listener-interrupts -lwiringPi`

## To Use

- Start the Gameboy running the cart-dumper ROM
- Connect it to the gameboy with a breakout board connected to pins 29-39
- Start the listener on the Raspberry Pi
- Swap Gameboy cartridges
- Press Start to begin the dump
- When the dump is complete press Escape in the listener terminal window

