void enableInterrupts();
int interrupt(int n, int ax, int bx, int cx, int dx);

void main() 
{
  int i = 0;

  enableInterrupts();

  for (i = 1; i < 3; i++) {
    interrupt(0x21, 20, (int)"hello world", i, 0);
  }

  interrupt(0x21, 5, 0, 0, 0);
}
