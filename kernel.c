int mod(int a, int b);
void readSector(char *buffer, int sector);
void printChar(char c);
void printString(char* str);
void readString(char buffer[]); 
void handleInterrupt21(int ax, int bx, int cx, int dx);

void main()
{
	char line[80]; 
	makeInterrupt21(); 
	interrupt(0x21,1,line,0,0); 
	interrupt(0x21,0,line,0,0); 

	while (1);
}

void handleInterrupt21(int ax, int bx, int cx, int dx)
{
	switch(ax) {
		case 0 :
			printString(bx);
			break;
		case 1 :
			readString(bx);
			break;
		case 2 :
			readSector(bx,cx);
			break;
		default :
			printString("Invalid value in reg AX");
	}
}

void readSector(char *buffer, int sector)
{
	int relativeSector = mod(sector, 18) + 1;
	int track = sector / 36;
	int head = mod((sector / 18), 2);
	int floppyDevice = 0;
	interrupt(0x13, 513, buffer, (track*256 + relativeSector), (head*256 + floppyDevice));
}

int mod(int a, int b)
{
	while(a > b)
	{
		a -= b;
	}
	return a;
}

void printChar(char c)
{	
	interrupt(0x10, 0xe*256 + c, 0, 0, 0); 
}

void printString(char* str)
{
	while (*str != '\0')
	{
		printChar(*str);
		str++;
	}
}

void readString(char buffer[])
{
	int i;
	char letter;

	for (i = 0; i < 79; i++)
	{
		letter = interrupt(0x16, 0, 0, 0, 0); 
		if ((letter == 0x8) && (i > 0)) {
			i -= 2;	
		} else if (letter == 0xd) {
			buffer[i] = 0xa;
			buffer[i+1] = 0x0;
			printChar(0xa);
			return; 
		} else {
			buffer[i] = letter;
		}
		
		printChar(letter);
	}
}
