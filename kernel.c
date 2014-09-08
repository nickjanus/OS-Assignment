#define DIR_SECTOR 2 
#define MAP_SECTOR 1
#define SECTOR_SIZE 512
#define ENTRY_SIZE 32
#define NAME_SIZE 6

int interrupt(int number, int ax, int bx, int cx, int dx);
int mod(int a, int b);
void readSector(char *buffer, int sector);
void writeSector(char *buffer, int sector);
void printChar(char c);
void printString(char* str);
void readString(char buffer[]); 
void handleInterrupt21(int ax, int bx, int cx, int dx);
void directory();
void readFile (char* filename, char outbuf[]);
void deleteFile(char* filename);
int getDirIndex(char* filename);
void loadDirectory();
void makeInterrupt21();
void main()
{
  makeInterrupt21(); 
  interrupt(0x21, 3, 0,0,0); 
  interrupt(0x21, 4, (int)"messag", 0, 0); 
  interrupt(0x21, 3, 0,0,0);
  while (1);
}

void handleInterrupt21(int ax, int bx, int cx, int dx)
{
  switch(ax) {
    case 0 :
      printString((char *)bx);
      break;
    case 1 :
      readString((char *)bx);
      break;
    case 2 :
      readSector((char *)bx,cx);
      break;
    case 3 :
      directory();
      break;
    case 4 : 
      deleteFile((char *)bx);
      break;
    default :
      printString("Invalid value in reg AX");
  }
}

void loadDirectory(char* buffer) {
  readSector(buffer,DIR_SECTOR);
}

void loadMap(char* buffer) {
  readSector(buffer,MAP_SECTOR);
}

void saveDirectory(char* buffer) {
  writeSector(buffer,DIR_SECTOR);
}

void saveMap(char* buffer) {
  writeSector(buffer,MAP_SECTOR);
}

void directory() {
  int x, y;
  char directory[SECTOR_SIZE];
  loadDirectory(directory);

  for (x = 0; x < SECTOR_SIZE; x += ENTRY_SIZE) {
    if (directory[x] != 0) {
      for (y = 0; y < NAME_SIZE; y++) {
        printChar(directory[x + y]);
      }

      printChar(0xa); //next entry on a new line
      printChar(0xd); //start at beginning of line
    }
  }
}

void readFile (char* filename, char outbuf[]) {
  
}
int getDirIndex(char* filename) {
  int x, y, matches;
  char fileChar, entryChar;
  char directory[SECTOR_SIZE];
  loadDirectory(directory);

  for (x = 0; x < SECTOR_SIZE; x += ENTRY_SIZE) {
    matches = 1;

    for (y = 0; y < NAME_SIZE; y++) {
      fileChar = *(filename + y);
      entryChar = directory[x + y];

      if (fileChar != entryChar) {
        matches = 0;
        break;
      }
    }

    if (matches) {
      return x;
    } 
  }
}

void deleteFile(char* filename) {
  int x, y, matches;
  char fileChar, entryChar;
  char map[SECTOR_SIZE];
  char directory[SECTOR_SIZE];
  static char zeroSector[SECTOR_SIZE];
  loadDirectory(directory);
  loadMap(map);

  for (x = 0; x < SECTOR_SIZE; x += ENTRY_SIZE) {
    matches = 1;

    for (y = 0; y < NAME_SIZE; y++) {
      fileChar = *(filename + y);
      entryChar = directory[x + y];

      if (fileChar != entryChar) {
        matches = 0;
        break;
      }
    }

    if (matches) {
      for (y = 0; y < ENTRY_SIZE; y++) {
        entryChar = directory[x + y];
        if ((y >= NAME_SIZE) && (entryChar != 0)) {
          map[(int)entryChar] = 0;
          writeSector(zeroSector, (int)entryChar);
        }
            
        directory[x+y] = 0;
      }
      saveMap(map);
      saveDirectory(directory);
      return;
    }
  }
}

void writeSector(char *buffer, int sector)
{
  int relativeSector = mod(sector, 18) + 1;
  int track = sector / 36;
  int head = mod((sector / 18), 2);
  int floppyDevice = 0;
  interrupt(0x13, (3 * 256 + 1), (int)buffer, (track*256 + relativeSector), (head*256 + floppyDevice));
}

void readSector(char *buffer, int sector)
{
  int relativeSector = mod(sector, 18) + 1;
  int track = sector / 36;
  int head = mod((sector / 18), 2);
  int floppyDevice = 0;
  interrupt(0x13, (2 * 256 + 1), (int)buffer, (track*256 + relativeSector), (head*256 + floppyDevice));
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
      printChar(0xa); //next entry on a new line
      printChar(0xd); //start at beginning of line
      return; 
    } else {
      buffer[i] = letter;
    }
    
    printChar(letter);
  }
}
