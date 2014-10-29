void enableInterrupts();
int interrupt(int n, int ax, int bx, int cx, int dx);

void main() 
{
  int i = 0;

  enableInterrupts();

  interrupt(0x21, 0, "\nSending message...\n", 0, 0);
  //spam all the processes
  for (i = 0; i < 8; i++) {
    interrupt(0x21, 11, (char*)"hello world", i, 0);
  }
}
