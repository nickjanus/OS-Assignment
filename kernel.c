void printChar(char c);
void printString(char* str);
void readString(char buffer[]); 

void main()
{
	char line[80]; 
	printString("Enter a line: "); 
	readString(line); 
	printString(line);
	while (1);
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
			printChar(letter);
			return; 
		} else {
			buffer[i] = letter;
		}
		
		printChar(letter);
	}
}
