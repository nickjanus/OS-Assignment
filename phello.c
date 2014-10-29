void main() 
{
  int i,j,k;
  int silly=0;
  char CRLF[3];

  enableInterrupts();

  CRLF[0] = 0xa;
  CRLF[1] = 0xd;
  CRLF[2] = 0;

  for (i=0;i<1000;++i){
    interrupt(0x21, 0, "hello world", 0, 0);
    interrupt(0x21, 0, CRLF, 0, 0);

    for(k=0;k<5;++ k) {
      for(j=0;j<30000;++j) {
        silly +=j;
        if (silly > 100) silly =0;
      }
    }
  }
}
