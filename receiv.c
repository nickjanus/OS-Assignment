void enableInterrupts();
int interrupt(int n, int ax, int bx, int cx, int dx);

void main() 
{
  char buffer[100];

  enableInterrupts();

  //check messages
  interrupt(0x21, 21, (int)buffer, 0, 0);
  interrupt(0x21, 0, (int)buffer, 0, 0);
  interrupt(0x21, 5, 0, 0, 0);
}
