#include <stdio.h>
#include <wiringPi.h>

// data out
#define SO_PIN 6
// data in
#define SI_PIN 13
// inbound clock
#define SC_PIN 26

int volatile bitCounter = 0;
int volatile currentByte = 0;
unsigned volatile long lastReceive = 0;

void clock() {
  delayMicroseconds(20);
  int bit = digitalRead(SI_PIN);

  printf("      -%d- (%dus)\n", bit, (micros() - lastReceive));

  if ( lastReceive > 0 && micros() - lastReceive > 250 )  // too long, must be a new sequence (takes about 120 microsecs for a bit)
    {
      printf("    reset, lr=(%dus), bc=[%d], cb=%2x\n", (micros() - lastReceive), bitCounter, currentByte);
      bitCounter = 0;
      currentByte = 0;      
    }
  
  if ( bit == HIGH ){
    currentByte |= (bit << (7-bitCounter) );
  }
    
  if ( bitCounter == 7 )
  {
if (currentByte != 0x55){
    printf("##Byte: %2x\n", currentByte);   
}else {
    printf("  Byte: %2x\n", currentByte);    
}
    currentByte = 0;
    bitCounter = 0;
  }
  else {
    bitCounter++;
  }
    
  lastReceive = micros();
}

int main (void)
{
    printf("wiringPi setting up..."); 
	wiringPiSetupGpio();
    printf("wiringPi set up\n"); 
	
    printf("pins setting..."); 
	pinMode(SO_PIN, OUTPUT);
	pinMode(SI_PIN, INPUT);
	//pullUpDnControl(SI_PIN, PUD_OFF);
	pinMode(SC_PIN, INPUT);
	//pullUpDnControl(SI_PIN, PUD_OFF);
    printf("pins set\n"); 

	// Attempt to get hi privs
    printf("attempt privs..."); 
	piHiPri(75);
    printf("privs set\n");  

    printf("attaching isr...");  
	wiringPiISR(SC_PIN, INT_EDGE_FALLING, clock);
    printf("attached isr\n");

    printf("looping...");
	while (1){
		delayMicroseconds(100);
	}
    printf("looped\n");

	return 0;
}

