#include <stdio.h>
#include <wiringPi.h>
#include <curses.h>

// data out
#define VDD_PIN 5
// data out
#define SO_PIN 6
// data in
#define SI_PIN 13
// outbound clock
#define SD_PIN 19
// inbound clock
#define SC_PIN 26

// Byte we send out on recieving a byte
#define OKAY_BYTE 0xDE

// 16k byte read buffer (1 bank size)
#define READ_BUFFER_SIZE 0x4000

// Cart header positions
#define CART_TITLE 0x0134
#define CART_ROM_SIZE 0x0148

void initWiringPi()
{
  printw("Setting up WiringPi...\n");
  wiringPiSetupGpio();

  printw(" - Pin mode setting...");
  pinMode(VDD_PIN, OUTPUT);
  pinMode(SO_PIN, OUTPUT);
  pinMode(SI_PIN, INPUT);
  pinMode(SD_PIN, OUTPUT);
  pinMode(SC_PIN, INPUT);
  // Write high to VDD, try to keep from messing up contrast?
  digitalWrite(VDD_PIN, HIGH);
  // Write high to outbound clock, triggers on low
  digitalWrite(SD_PIN, HIGH);
  printw("Done\n");

  // Attempt to get high privs
  printw(" - Priviledge raising...");
  if (piHiPri(0) == 0)
  {
    printw("Done\n");
  }
  else {
    printw("Failed\n");
  }

  printw("WiringPi set up\n");
}

void parseHeader(unsigned char * buffer)
{
  // Parse title from the header
  printw("Cartridge Title: ");
  unsigned char *ptr = buffer + CART_TITLE;
  for (int i = 0; i < 16; i++) {
    if (*ptr == '\0')
    {
      break;
    }

    printw("%c", *(ptr++));
  }
  printw("\n");

  // Calculate bytes to read from the header
  unsigned long bytesToRead = 0x8000 << *(buffer + CART_ROM_SIZE);
  printw("Bytes to read: %lu\n", bytesToRead);
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printf("Usage: %s <outputFile>\n", argv[0]);
    return 1;
  }

  // Init ncurses
  initscr();

  // Open file for write
  FILE *fp;
  fp = fopen(argv[1], "w+b");

  // Init WiringPi
  initWiringPi();

  // Flag for the transfer being started
  int started = 0;

  // Flag for the clock tick being noticed
  int clocked = 0;

  // Byte-reading vars
  unsigned char bitCounter = 0;
  unsigned char currentByte = 0;
  unsigned long lastReceive = 0;

  // Read buffer
  unsigned long totalBytesRead = 0;
  unsigned char buf[READ_BUFFER_SIZE];
  unsigned long bufIndex = 0;

  // Set ncurses timeout to nothing (to make the getch() non-blocking)
  timeout(0);

  // Keypress char to test if we want to break out the loop
  int keyChar;

  printw("Waiting for data... (press 'q' to stop)\n");
  while (1)
  {
    // Break the loop when ESC/q key pressed
    keyChar = getch();
    if (keyChar == 27 || keyChar == 'q')
    {
      break;
    }

    // End if it's been 2s since the last byte
    if (started != 0 && micros() - lastReceive > 2000)
    {
      printw("Finished transferring\n");
      break;
    }

    int clockPin = digitalRead(SC_PIN);
    if (clockPin == LOW && clocked == 0)
    {
      clocked = 1;
      // Put our current outbound bit on the wire bits
      digitalWrite(SO_PIN, (OKAY_BYTE >> (7 - bitCounter)) & 0x01);
    }
    else if (clockPin == HIGH && clocked == 1)
    {
      // rising edge bit check
      clocked = 0;

      // Using the Gameboy clock it should be ~120us per clock, any longer it's likely a new byte so reset
      if (lastReceive > 0 && micros() - lastReceive > 750)
      {
        if (bitCounter != 0 || currentByte != 0)
        {
          // If we reset while having partial byte data there's likely been a timing error
          printw(" - [ERROR] byte[%2x] reset, lr=(%dus), bc=[%d], cb=%2x\n", totalBytesRead, (micros() - lastReceive), bitCounter, currentByte);

          bitCounter = 0;
          currentByte = 0;
        }
      }

      int inBit = digitalRead(SI_PIN);

      if (inBit == HIGH)
      {
        currentByte |= (inBit << (7 - bitCounter));
      }

      bitCounter++;

      if (bitCounter == 8)
      {
        // When we have a full byte, put it in the write-buffer
        buf[bufIndex++] = currentByte;

        // Reset ready for next byte
        currentByte = 0;
        bitCounter = 0;

        if (totalBytesRead++ == 0x014F) {
          parseHeader(buf);
        }

        // When the buffer gets full, write it out to the file
        if (bufIndex == READ_BUFFER_SIZE)
        {
          fwrite(buf, sizeof(buf[0]), bufIndex, fp);
          bufIndex = 0;
        }
      }

      // Store the last recieve time in us to check for errors
      lastReceive = micros();

      started = 1; // flag transfer started
    }
  }

  // Write anything left in the buffer out
  if (bufIndex > 0)
  {
    fwrite(buf, sizeof(buf[0]), bufIndex, fp);
  }
  fclose(fp);

  printw("Finished recieving data\n");

  endwin();

  return 0;
}
