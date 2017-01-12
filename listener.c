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

#define OKAY_BYTE 0xDE

// 16 k rom/ram, so buffer of one of those
#define READ_BUFFER_SIZE 1024 * 16 * 2

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printf("Usage: %s <outputFile>\n", argv[0]);
    return 1;
  }

  // Open file for write
  FILE *fp;
  fp = fopen(argv[1], "w+b");

  printf("wiringPi setting up...");
  wiringPiSetupGpio();
  printf("wiringPi set up\n");

  printf("pins setting...");
  pinMode(VDD_PIN, OUTPUT);
  pinMode(SO_PIN, OUTPUT);
  pinMode(SI_PIN, INPUT);
  pinMode(SD_PIN, OUTPUT);
  pinMode(SC_PIN, INPUT);
  // Write high to VDD, try to keep from messing up contrast?
  digitalWrite(VDD_PIN, HIGH);
  // Write high to outbound clock, triggers on low
  digitalWrite(SD_PIN, HIGH);
  printf("pins set\n");

  // Attempt to get high privs
  printf("attempt privs...");
  piHiPri(75);
  printf("privs set\n");

  unsigned char buf[READ_BUFFER_SIZE];
  unsigned long readCount = 0;
  unsigned char bitCounter = 0;
  unsigned char currentByte = 0;
  unsigned long lastReceive = 0;

  int clocked = 0;

  printf("looping...\n");
  int c;
  initscr();
  timeout(0);
  while (1)
  {
		// Break the loop when ESC key pressed
    c = getch();
    if (c == 27)
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
					printf("[ERROR] byte[%2x] reset, lr=(%dus), bc=[%d], cb=%2x\n", readCount, (micros() - lastReceive), bitCounter, currentByte);
				}
				bitCounter = 0;
				currentByte = 0;
      }

      int inBit = digitalRead(SI_PIN);

      if (inBit == HIGH)
      {
				currentByte |= (inBit << (7 - bitCounter));
      }

      if (bitCounter == 7)
      {
				//printf("Byte: %2x\n", currentByte);
				buf[readCount] = currentByte;
				readCount++;

				currentByte = 0;
				bitCounter = 0;
      }
      else
      {
				bitCounter++;
      }

      lastReceive = micros();
    }
  }
  endwin();
  printf("looped\n");

	// Write out the buffer to the file
  fwrite(buf, sizeof(buf[0]), sizeof(buf) / sizeof(buf[0]), fp);
  fclose(fp);

  return 0;
}
