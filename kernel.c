#define DIR_SECTOR 2 
#define MAP_SECTOR 1
#define SECTOR_SIZE 512
#define ENTRY_SIZE 32
#define NAME_SIZE 6
#define MAX_FILE_SIZE 13312 

int interrupt(int number, int ax, int bx, int cx, int dx);
void putInMemory(int segment, int offset, char c);
int mod(int a, int b);
void readSector(char *buffer, int sector);
void writeSector(char *buffer, int sector);
void printChar(char c);
void printString(char* str);
void readString(char buffer[]); 
void printNewLine();
void handleInterrupt21(int ax, int bx, int cx, int dx);
void directory();
void readFile (char* filename, char outbuf[]);
void writeFile (char* filename, char inbuf[]);
void deleteFile(char* filename);
int getDirIndex(char* filename);
void loadDirectory();
void loadMap();
void makeInterrupt21();
int getEmptyDirIndex(char* directory);
int getEmptySector(char* map);
int getFileSize(char* name);
void executeProgram(char* name, int segment);
void launchProgram(int segment);
void terminate();
void makeTimerInterrupt();
void returnFromTimer(int segment, int sp);
void handleTimerInterrupt(int segment, int sp);

void main()
{
  makeInterrupt21();  //create system call interrupt 
  makeTimerInterrupt(); //create timer interrupt for scheduling
  interrupt(0x21, 9, (int)"shell\0", 0x2000, 0);
}

void handleTimerInterrupt(int segment, int sp) {
  interrupt(0x10, 0xe*256 + 'p', 0, 0, 0); printString("tic");

  returnFromTimer(segment, sp);
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
    case 5 : 
      terminate();
      break;
    case 6 : 
      readFile((char *)bx, (char *)cx);
      break;
    case 7 : 
      writeSector((char *)bx,cx);
      break;
    case 8 : 
      writeFile((char *)bx, (char *)cx);
      break;
    case 9 : 
      executeProgram((char *)bx, cx);
      break;
    default :
      printString("Invalid value in reg AX\n");
  }
}

void terminate() {
  launchProgram(0x2000);
}

void executeProgram(char* name, int segment) {
  int x, y, programSize;
  char program[MAX_FILE_SIZE];
  programSize = getFileSize(name);

  if ((segment <= 0x1000) || (segment > 0xA000) || (mod(segment, 0x1000) != 0) ) {
    printString("Error: Invalid Segment!\n");
    return;
  }

  readFile(name, program);
  for (x = 0; x < programSize; x += SECTOR_SIZE) {
      for(y = 0; y < SECTOR_SIZE; y++) {
        putInMemory(segment, x + y, *(program + x + y));
      }
  }

  launchProgram(segment);
}

int getFileSize(char* name) {
  int x, index, size = 0;
  char* entry;
  char directory[SECTOR_SIZE];

  loadDirectory(directory);
  index = getDirIndex(name);

  for (x = 6; x < ENTRY_SIZE; x++) {
    if (*(directory + index + x) != 0) {
      size++;
    }
  }

  return size * 0x200;
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
      printNewLine();
    }
  }
}

void printNewLine() {
      printChar(0xa); //next entry on a new line
      printChar(0xd); //start at beginning of line
}

void writeFile (char* filename, char inbuf[]) {
  int x, sector, dirIndex, eolEncountered = 0;
  char directory[SECTOR_SIZE];
  char map[SECTOR_SIZE];

  loadDirectory(directory);
  loadMap(map);
  dirIndex = getEmptyDirIndex(directory);
  if (dirIndex == -1) {printString("ERROR: Directory is full\n"); return;}

  //write name into directory
  for (x = 0; x < NAME_SIZE; x++) {
    if ((eolEncountered) || (*(filename+x) == 0xA)) {
      eolEncountered = 1;
      directory[dirIndex + x] = 0;
    } else {
      directory[dirIndex + x] = *(filename + x);
    }
  }

  dirIndex += NAME_SIZE;
  //write file to sectors and record in map/directory
  //size of the file is unknown, so write up to max
  //break if a sector starts as null...
  for (x = 0; x < MAX_FILE_SIZE; x += SECTOR_SIZE) {
    sector = getEmptySector(map);
    if (sector == -1) {printString("ERROR: Out of disk space\n"); return;}
    
    directory[dirIndex] = sector;
    dirIndex++;
    map[sector] = 0xFF;

    if (*(inbuf + x) != 0) { 
      writeSector(inbuf + x, sector);
    } else {
      break;
    }
  }

  saveDirectory(directory);
  saveMap(map);
}

int getEmptySector(char* map) {
  int x, sector;
  sector = -1;

  for (x = 0; x < SECTOR_SIZE; x++) {
    if (*(map + x) == 0) {
      sector = x;
      break;
    }
  }
  return sector;
}

//returns index of directory entry
int getEmptyDirIndex(char* directory) {
  int x, dirIndex;
  dirIndex = -1;
  
  for (x = 0; x < SECTOR_SIZE; x += ENTRY_SIZE) {
    if(*(directory + x) == 0) {
      dirIndex = x;
      break;
    }
  }
  return dirIndex;
}

void readFile (char* filename, char outbuf[]) {
  int x, index;
  char entryChar;
  char directory[SECTOR_SIZE];

  loadDirectory(directory);
  index = getDirIndex(filename);
  if (index == -1) {printString("ERROR: No such file in Directory\n"); return;}

  for(x = NAME_SIZE; x < ENTRY_SIZE; x++) {
    entryChar = directory[index + x];
    if (entryChar != 0) {
      readSector((outbuf + (x - NAME_SIZE) * SECTOR_SIZE), (int)entryChar);
    }
  }
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

      if ((fileChar == 0xA) && (entryChar == 0)) {
        break;
      } else if (fileChar != entryChar) {
        matches = 0;
        break;
      }
    }

    if (matches) {
      return x;
    } 
  }

  return -1;
}

void deleteFile(char* filename) {
  int y, index;
  char entryChar;
  char map[SECTOR_SIZE];
  char directory[SECTOR_SIZE];
  static char zeroSector[SECTOR_SIZE];
  loadDirectory(directory);
  loadMap(map);
  index = getDirIndex(filename);
  if (index == -1) {printString("ERROR: No such file in Directory\n"); return;}

  if (index != -1) {
    for (y = 0; y < NAME_SIZE; y++) {
      directory[index+y] = 0;
    }

    for (y = NAME_SIZE; y < ENTRY_SIZE; y++) {
      entryChar = directory[index + y];
      if ((entryChar != 0)) {
        map[(int)entryChar] = 0;
        writeSector(zeroSector, (int)entryChar);
      }
          
      directory[index+y] = 0;
    }
    saveMap(map);
    saveDirectory(directory);
    return;
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
  while(a >= b)
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
    if (*str == '\n') {
      printNewLine();
    } else {
      printChar(*str);
    }
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
