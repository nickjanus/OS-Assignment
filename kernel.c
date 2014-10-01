#define DIR_SECTOR 2 
#define MAP_SECTOR 1
#define SECTOR_SIZE 512
#define ENTRY_SIZE 32
#define NAME_SIZE 6
#define MAX_FILE_SIZE 13312 
#define MAX_PROCESSES 8
#define SEGMENT_SIZE 0x1000
#define KERNEL_SEGMENT 0x1000
#define USER_SPACE_OFFSET 0x2000
#define INIT_STACK_POINTER 0xFF00
#define INT_MAX_LENGTH 5 //ints are 2 bytes, largest around 32,000
typedef struct procEntry {
  int active;
  int stackPointer;
} procEntry;

int interrupt(int number, int ax, int bx, int cx, int dx);
void putInMemory(int segment, int offset, char c);
int mod(int a, int b);
void readSector(char *buffer, int sector);
void writeSector(char *buffer, int sector);
void printChar(char c);
void printString(char* str);
void printInt(int n);
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
void executeProgram(char* name);
void initializeProgram();
void terminate();
void makeTimerInterrupt();
void returnFromTimer(int segment, int sp);
void handleTimerInterrupt(int segment, int sp);
int getFreeProcEntry();
void setKernelDataSegment();
void restoreDataSegment();
void main2();

void main() {main2();}
procEntry ProcessTable[MAX_PROCESSES];
int CurrentProcess;
int KernelStackPointer;

void main2()
{
  //setup proc table
  int i;
  for(i = 0; i < MAX_PROCESSES; i++) {
    ProcessTable[i].active = 0;
    ProcessTable[i].stackPointer = INIT_STACK_POINTER;
  }
  CurrentProcess = -1; //start at beginning of proc table at first interrupt

  //set up interrupts
  makeInterrupt21();  //create system call interrupt 
  makeTimerInterrupt(); //create timer interrupt for scheduling

  //launch shell
  executeProgram("shell\0");
}

void handleTimerInterrupt(int segment, int sp) {
  int x, nextSegment = -1, nextStackPointer = -1;

  setKernelDataSegment();

if (segment == 0x1000) {
  putInMemory(0xB000, 0x8162, 'K');
  putInMemory(0xB000, 0x8163, 0x7);
}

else if (segment == 0x2000) {
  putInMemory(0xB000, 0x8164, '0');
  putInMemory(0xB000, 0x8165, 0x7);
}
else if (segment == 0x3000) {
  putInMemory(0xB000, 0x8166, '1');
  putInMemory(0xB000, 0x8167, 0x7);
}
else {
  putInMemory(0xB000, 0x8160, 'X');
  putInMemory(0xB000, 0x8161, 0x7);
}

//printString("\nHandling timer Interrupt...\n");
  //stow kernel sp if kernel is current proc
  if(CurrentProcess == -1) {
    KernelStackPointer = sp;
  } else {
    ProcessTable[CurrentProcess].stackPointer = sp;
  }

//printString("\nCurrent proc: "); printInt(CurrentProcess);
  for (x = CurrentProcess + 1; x <= MAX_PROCESSES; x++) {
    if(ProcessTable[x].active) {
      nextSegment = x * SEGMENT_SIZE + USER_SPACE_OFFSET;
      nextStackPointer = ProcessTable[x].stackPointer;
//printString("\nNext proc: "); printInt(x);
      CurrentProcess = x;
      break;
    }
  }

  //kernel's turn to run?
  if(nextSegment == -1) {
    nextStackPointer = KernelStackPointer;
    nextSegment = KERNEL_SEGMENT;
    CurrentProcess = -1;
//printString("\nNext proc: "); printInt(-1);
  }
  
//printString("\nCurrent sp: "); printInt(sp);
//printString("\nNext sp: "); printInt(nextStackPointer);
  restoreDataSegment();
  returnFromTimer(nextSegment, nextStackPointer);
}

void handleInterrupt21(int ax, int bx, int cx, int dx)
{
  setKernelDataSegment();

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
      executeProgram((char *)bx);
      break;
    default :
      printString("Invalid value in reg AX\n");
  }

  restoreDataSegment();
}

void terminate() {
  setKernelDataSegment();
  ProcessTable[CurrentProcess].active = 0;
  ProcessTable[CurrentProcess].stackPointer = INIT_STACK_POINTER;
  restoreDataSegment(); 
  while (1);
}

void executeProgram(char* name) {
  int x, y, programSize, procEntry, segment;
  char program[MAX_FILE_SIZE];

  setKernelDataSegment();
  programSize = getFileSize(name);

  procEntry = getFreeProcEntry();
  segment = procEntry * SEGMENT_SIZE + USER_SPACE_OFFSET;
  if (segment == 0) {printString("Error: Max number of processes reached.\n"); return;}

  readFile(name, program);
  for (x = 0; x < programSize; x += SECTOR_SIZE) {
      for(y = 0; y < SECTOR_SIZE; y++) {
        putInMemory(segment, x + y, *(program + x + y));
      }
  }
  initializeProgram(segment);
  ProcessTable[procEntry].active = 1;
  restoreDataSegment();
}

int getFreeProcEntry() {
  int x, freeProcEntry = -2;
  setKernelDataSegment();

  for (x = 0; x < MAX_PROCESSES; x++) {
    if (ProcessTable[x].active == 0) {
      freeProcEntry = x; 
      break;
    }
  }

  restoreDataSegment();
  return freeProcEntry;
}

int getFileSize(char* name) {
  int x, index, size = 0;
  char* entry;
  char directory[SECTOR_SIZE];

  setKernelDataSegment();
  loadDirectory(directory);
  index = getDirIndex(name);

  for (x = 6; x < ENTRY_SIZE; x++) {
    if (*(directory + index + x) != 0) {
      size++;
    }
  }

  restoreDataSegment();
  return size * SECTOR_SIZE;
}

void loadDirectory(char* buffer) {
  setKernelDataSegment();
  readSector(buffer,DIR_SECTOR);
  restoreDataSegment();
}

void loadMap(char* buffer) {
  setKernelDataSegment();
  readSector(buffer,MAP_SECTOR);
  restoreDataSegment();
}

void saveDirectory(char* buffer) {
  setKernelDataSegment();
  writeSector(buffer,DIR_SECTOR);
  restoreDataSegment();
}

void saveMap(char* buffer) {
  setKernelDataSegment();
  writeSector(buffer,MAP_SECTOR);
  restoreDataSegment();
}

void directory() {
  int x, y;
  char directory[SECTOR_SIZE];

  setKernelDataSegment();
  loadDirectory(directory);

  for (x = 0; x < SECTOR_SIZE; x += ENTRY_SIZE) {
    if (directory[x] != 0) {
      for (y = 0; y < NAME_SIZE; y++) {
        printChar(directory[x + y]);
      }
      printNewLine();
    }
  }
  restoreDataSegment();
}

void printNewLine() {
  setKernelDataSegment();
  printChar(0xa); //next entry on a new line
  printChar(0xd); //start at beginning of line
  restoreDataSegment();
}

void writeFile (char* filename, char inbuf[]) {
  int x, sector, dirIndex, eolEncountered = 0;
  char directory[SECTOR_SIZE];
  char map[SECTOR_SIZE];

  setKernelDataSegment();
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
  restoreDataSegment();
}

int getEmptySector(char* map) {
  int x, sector;

  setKernelDataSegment();
  sector = -1;

  for (x = 0; x < SECTOR_SIZE; x++) {
    if (*(map + x) == 0) {
      sector = x;
      break;
    }
  }
  restoreDataSegment();
  return sector;
}

//returns index of directory entry
int getEmptyDirIndex(char* directory) {
  int x, dirIndex;

  setKernelDataSegment();
  dirIndex = -1;
  
  for (x = 0; x < SECTOR_SIZE; x += ENTRY_SIZE) {
    if(*(directory + x) == 0) {
      dirIndex = x;
      break;
    }
  }
  restoreDataSegment();
  return dirIndex;
}

void readFile (char* filename, char outbuf[]) {
  int x, index;
  char entryChar;
  char directory[SECTOR_SIZE];

  setKernelDataSegment();
  loadDirectory(directory);
  index = getDirIndex(filename);
  if (index == -1) {printString("ERROR: No such file in Directory\n"); return;}

  for(x = NAME_SIZE; x < ENTRY_SIZE; x++) {
    entryChar = directory[index + x];
    if (entryChar != 0) {
      readSector((outbuf + (x - NAME_SIZE) * SECTOR_SIZE), (int)entryChar);
    }
  }

  restoreDataSegment();
}

int getDirIndex(char* filename) {
  int x, y, index, matches;
  char fileChar, entryChar;
  char directory[SECTOR_SIZE];

  setKernelDataSegment();
  loadDirectory(directory);

  for (x = 0; x < SECTOR_SIZE; x += ENTRY_SIZE) {
    matches = 1;
    index = -1;

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
      index = x;
      break;
    } 
  }

  restoreDataSegment();
  return index;
}

void deleteFile(char* filename) {
  int y, index;
  char entryChar;
  char map[SECTOR_SIZE];
  char directory[SECTOR_SIZE];
  static char zeroSector[SECTOR_SIZE];

  setKernelDataSegment();
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
  }
  restoreDataSegment();
}

void writeSector(char *buffer, int sector)
{
  int relativeSector = mod(sector, 18) + 1;
  int track = sector / 36;
  int head = mod((sector / 18), 2);
  int floppyDevice = 0;

  setKernelDataSegment();
  interrupt(0x13, (3 * 256 + 1), (int)buffer, (track*256 + relativeSector), (head*256 + floppyDevice));
  restoreDataSegment();
}

void readSector(char *buffer, int sector)
{
  int relativeSector = mod(sector, 18) + 1;
  int track = sector / 36;
  int head = mod((sector / 18), 2);
  int floppyDevice = 0;

  setKernelDataSegment();
  interrupt(0x13, (2 * 256 + 1), (int)buffer, (track*256 + relativeSector), (head*256 + floppyDevice));
  restoreDataSegment();
}

int mod(int a, int b)
{
  setKernelDataSegment();

  while(a >= b)
  {
    a -= b;
  }

  restoreDataSegment();
  return a;
}

void printChar(char c)
{  
  setKernelDataSegment();
  interrupt(0x10, 0xe*256 + c, 0, 0, 0); 
  restoreDataSegment();
}

void printInt(int n) {
  int quotient, remainder, length, x;
  int digits[INT_MAX_LENGTH];
  //432 would be stored in digits as [0,0,4,3,2]

  setKernelDataSegment();
  x = INT_MAX_LENGTH - 1;
  length = 0;
  do {
    quotient = n/10;
    remainder = mod(n,10);
    digits[x] = remainder + 48;
    length++;
    n = quotient;
    x--;
  } while (quotient != 0);

  for (x = length; x > 0 ; x--) {
    printChar(digits[INT_MAX_LENGTH - x]);
  }
  restoreDataSegment();
}

void printString(char* str)
{
  setKernelDataSegment();
  while (*str != '\0')
  {
    if (*str == '\n') {
      printNewLine();
    } else {
      printChar(*str);
    }
    str++;
  }
  restoreDataSegment();
}

void readString(char buffer[])
{
  int i;
  char letter;

  setKernelDataSegment();
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
      break; 
    } else {
      buffer[i] = letter;
    }
    
    printChar(letter);
  }

  restoreDataSegment();
}
