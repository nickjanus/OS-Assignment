#define COUNT(x) (sizeof(x) / sizeof(x[0])) 

void putChar(long colAddr, long rowAddr, char character);

void main()
{
	putsChar(0xB000, 0x8140, "Hello World")
	while (1);
}

void putChar(long colAddr, long rowAddr, char character)
{
	putInMemory(colAddr, rowAddr, character); 
	putInMemory(colAddr, (rowAddr + 0x1), 0x7); 
}

void putChars(long colAddr, long rowAddr, char chars[])
{
	int i;
	for(i = 0; i < COUNT(chars); i++)
	{
		putChar((colAddr + i * 0xA0),rowAddr,chars[i]);
	}
}
