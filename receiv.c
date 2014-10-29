void enableInterrupts();
int interrupt(int n, int ax, int bx, int cx, int dx);

void main() 
{
  char buffer[100];

  enableInterrupts();

  //check messages
  interrupt(0x21, 0, "\nChecking messages...\n", 0, 0);
  interrupt(0x21, 12, buffer, 0, 0);
  interrupt(0x21, 0, buffer, 0, 0);
}
