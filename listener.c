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
  if (piHiPri(75) == 0)
  {
    printw("Done\n");
  }
  else {
    printw("Failed\n");
  }
  
  printw("WiringPi set up\n");
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

  // Read buffer
  unsigned char buf[READ_BUFFER_SIZE];
  unsigned long readCount = 0;

  // Flag for the clock tick being noticed
  int clocked = 0;

  // Byte-reading vars
  unsigned char bitCounter = 0;
  unsigned char currentByte = 0;
  unsigned long lastReceive = 0;

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
          printw(" - [ERROR] byte[%2x] reset, lr=(%dus), bc=[%d], cb=%2x\n", readCount, (micros() - lastReceive), bitCounter, currentByte);

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
        buf[readCount++] = currentByte;

        // Reset ready for next byte
        currentByte = 0;
        bitCounter = 0;
      }

      // Store the last recieve time in us to check for errors
      lastReceive = micros();
    }
    
    // When the buffer gets full, write it out to the file
    if (readCount == READ_BUFFER_SIZE) 
    {
      printw(" - Writing buffer\n");
      fwrite(buf, sizeof(buf[0]), readCount, fp);
      readCount = 0;
    }
  }

  // Write anything left in the buffer out
  if (readCount > 0)
  {
    fwrite(buf, sizeof(buf[0]), readCount, fp);
  }
  fclose(fp);
  
  printw("Finished recieving data\n");

  endwin();

  return 0;
}
