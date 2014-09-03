int mod(int a, int b);
void readSector(char *buffer, int sector);
void printChar(char c);
void printString(char* str);
void readString(char buffer[]); 

void main()
{
	char buffer[512]; 
	readSector(buffer, 30); 
	printString(buffer); 
	while (1);
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
			i--;	
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
