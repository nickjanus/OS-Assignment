#define ROOT_DIR_SECTOR 2 
#define MAP_SECTOR 1
#define SECTOR_SIZE 512
#define NAME_SIZE 6
#define HEADER_SIZE 7 //Number of name and type bytes in directory
#define TYPE_INDEX HEADER_SIZE - 1
#define ENTRY_SIZE 32
#define DIR_ENTRY 1 //denotes type of entry in directory header
#define FILE_ENTRY 2 //ditto
#define MAX_FILE_SIZE 12800
#define SECTOR_USED 0xFF
#define MAX_PROCESSES 7
#define SEGMENT_SIZE 0x1000
#define KERNEL_SEGMENT 0x1000
#define MESSAGE_SEGMENT 0x2000
#define USER_SPACE_OFFSET 0x3000
#define INIT_STACK_POINTER 0xFF00
#define INT_MAX_LENGTH 5 //ints are 2 bytes, largest around 32,000
#define MESSAGE_LENGTH 100
#define MSG_AGE_INDEX MESSAGE_LENGTH
typedef struct procEntry {
  int active;
  int waiting;
  int stackPointer;
} procEntry;

int interrupt(int number, int ax, int bx, int cx, int dx);
void putInMemory(int segment, int offset, char c);
char readFromMemory(int segment, int offset);
int mod(int a, int b);
void readSector(char *buffer, int sector);
void writeSector(char *buffer, int sector);
void printChar(char c);
void printString(char* str);
void printInt(int n);
void readString(char buffer[]); 
void printNewLine();
void handleInterrupt21(int ax, int bx, int cx, int dx);
void directory(char* dirName);
void readFile (char* filename, char outbuf[]);
void writeFile (char* filename, char inbuf[]);
void deleteFile(char* filename);
int getDirIndex(char* filename, char* directory);
int getDirSector(char* name);
void loadDirectory(char* buffer, char* name);
void loadRootDirectory(char* buffer);
void loadMap(char* buffer);
void saveDirectory(char* buffer, char* name);
void saveRootDirectory(char* buffer);
void saveMap(char* buffer);
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
void killProcess(int proc);
void sendMessage(char* buffer, int receiver);
void getMessage(char* buffer);
int getMsgAddress(int receiver, int sender);
int getMsgAge(int receiver, int sender);
int makeDir(char* name);
int getDir(char* name);
void parseFileName(char* name, char* top, char* sub);
void copyString(char* from, char* to, int length);
void clearString(char* str, int length);
void moveString(char* from, char* to, int length);
int initEntry(char* filename, char* directory, int entryType);
void main2();

void main() {main2();}

//globals
procEntry ProcessTable[MAX_PROCESSES];
int CurrentProcess;

void main2()
{
  //setup proc table
  int i;
  for(i = 0; i < MAX_PROCESSES; i++) {
    ProcessTable[i].active = 0;
    ProcessTable[i].waiting = 0;
    ProcessTable[i].stackPointer = INIT_STACK_POINTER;
  }
  CurrentProcess = -1; 

  //set up interrupts
  makeInterrupt21();  //create system call interrupt 
  makeTimerInterrupt(); //create timer interrupt for scheduling

  //launch shell
  executeProgram("shell\0");
  while(1);//make sure the stack pointer for main execution stays here
}

//returns sector number of directory after creation
int makeDir(char* name) {
  char map[SECTOR_SIZE], directory[SECTOR_SIZE];
  int dirIndex, freeSector = 0;
  loadRootDirectory(directory);
  loadMap(map);

  dirIndex = initEntry(name, directory, DIR_ENTRY);
  freeSector = getEmptySector(map);
  *(directory + dirIndex) = freeSector;
  *(map + freeSector) = SECTOR_USED;
  saveRootDirectory(directory);
  saveMap(map);
  return freeSector;
}

int getDirSector(char* name) {
  int index, sector = ROOT_DIR_SECTOR;
  char rootDirectory[SECTOR_SIZE];
  loadRootDirectory(rootDirectory);
  index = getDirIndex(name, rootDirectory);
  sector = *(rootDirectory + index + HEADER_SIZE);
  return sector;
}

//top and sub should be as long as NAME_LENGTH
//name should be in the form: /top/sub, top/sub, /file or file
void parseFileName(char* name, char* top, char* sub) {
  int i = 0, x = 0, z;
  char *id;
  *top = '/';
  id = sub;

  if (*name == '/') {i++;}
  while((*(name + i) != '\0') && (*(name + i) != 0xA)) {
    if (*(name + i) == '/') {
      moveString(id, top, NAME_SIZE);
      for(z = x; z < NAME_SIZE && z > 0; z++) {*(top + z) = 0;}
      x = 0;
    } else {
      *(id + x) = *(name + i);
      x++; 
    }
    i++;
  }
  for(z = x; z < NAME_SIZE; z++) {*(sub + z) = 0;}
}

void copyString(char* from, char* to, int length) {
  int i;
  for(i = 0; i < length; i++) {
    *(to + i) = *(from + i);
  }
}

void clearString(char* str, int length) {
  int i;
  for(i = 0; i < length; i++) {
    *(str + i) = 0;
  }
}

void moveString(char* from, char* to, int length) {
  copyString(from, to, length);
  clearString(from, length);
}

int getMsgAddress(int receiver, int sender) {
  int addr = 0;
  addr = receiver * MAX_PROCESSES * (MESSAGE_LENGTH + 1); //+1 to track age
  addr += sender * (MESSAGE_LENGTH + 1);
  return addr;
}

int getMsgAge(int receiver, int sender) {
  int addr = getMsgAddress(receiver, sender);
  return (int) readFromMemory(MESSAGE_SEGMENT, addr + MSG_AGE_INDEX);
}

void setMsgAge(int receiver, int sender, int age) {
  int addr = getMsgAddress(receiver, sender);
  putInMemory(MESSAGE_SEGMENT, addr + MSG_AGE_INDEX, age);
}

void sendMessage(char* buffer, int receiver) {
  int i = 0, bufferEmpty = 0, originalAge, addr;

  setKernelDataSegment();
  addr = getMsgAddress(receiver, CurrentProcess);

  //age other messages
  originalAge = getMsgAge(receiver, CurrentProcess);
  setMsgAge(receiver, CurrentProcess, 0); //message not ready for reading
  restoreDataSegment();

  if (originalAge != 1) { //otherwise status quo
    for(i=0; i < MAX_PROCESSES; i++) {
      setKernelDataSegment();
      if ((i != CurrentProcess) && (getMsgAge(receiver, i) > 0)) {
        setMsgAge(receiver, i, getMsgAge(receiver, i) + 1); 
      }
      restoreDataSegment();
    }
  }

  //copy message to first null character and write null after that
  for(i=0; i < MESSAGE_LENGTH; i++) {
    if ((*(buffer + i) == '\0') || bufferEmpty) {
      bufferEmpty = 1;
      putInMemory(MESSAGE_SEGMENT, addr + i, '\0');
    } else {
      putInMemory(MESSAGE_SEGMENT, addr + i, *(buffer + i));
    }
  }

  setKernelDataSegment();
  setMsgAge(receiver, CurrentProcess, 1); //message is ready
  if (ProcessTable[receiver].waiting) {
    ProcessTable[receiver].active = 1;
    ProcessTable[receiver].waiting = 0;
  }
  restoreDataSegment();
}

void getMessage(char* buffer) {
  int fromProc, maxAge = 0, i = 0, currentProc, addr;
  setKernelDataSegment();
  currentProc = CurrentProcess;
  restoreDataSegment();

  //get oldest message
  while(maxAge == 0) {
    for (i = 0; i < MAX_PROCESSES; i++) {
      if (getMsgAge(currentProc, i) > maxAge) {
        maxAge = getMsgAge(currentProc, i);
        fromProc = i;
      }
    }

    //set proc to waiting if no msg
    if (maxAge == 0) {
      setKernelDataSegment();
      ProcessTable[CurrentProcess].waiting = 1;
      ProcessTable[CurrentProcess].active  = 0;
      restoreDataSegment();
    }
  }
  addr = getMsgAddress(currentProc, fromProc);
  
  //copy message
  for (i = 0; i < MESSAGE_LENGTH; i++) {
    *(buffer + i) = readFromMemory(MESSAGE_SEGMENT, addr + i);
  }

  //"remove" message
  setMsgAge(currentProc, fromProc, 0);
}

//scheduler
void handleTimerInterrupt(int segment, int sp) {
  int x, index;

  setKernelDataSegment();
  for (x = 1; x <= MAX_PROCESSES; x++) {
    index = mod(x + CurrentProcess, MAX_PROCESSES);
    if(ProcessTable[index].active) {
      segment = index * SEGMENT_SIZE + USER_SPACE_OFFSET;
      if ((CurrentProcess != -1) && (ProcessTable[CurrentProcess].active)) {
        ProcessTable[CurrentProcess].stackPointer = sp;
      }
      sp = ProcessTable[index].stackPointer;
      CurrentProcess = index;
      break;
    }
  }
  restoreDataSegment();
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
      directory((char *)bx);
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
    case 10:
      killProcess(bx);
      break;
    case 11:
      makeDir((char *)bx);
      break;
    case 20:
      sendMessage((char *)bx, cx);
      break;
    case 21:
      getMessage((char *)bx);
      break;
    default :
      setKernelDataSegment();
      printString("Invalid value in reg AX\n");
      restoreDataSegment();
  }
}

void killProcess(int proc) {
  setKernelDataSegment();
  ProcessTable[proc].active = 0;
  ProcessTable[proc].waiting = 0;
  ProcessTable[proc].stackPointer = INIT_STACK_POINTER;
  restoreDataSegment();
}

void terminate() {
  int proc;
  setKernelDataSegment();
  proc = CurrentProcess;
  restoreDataSegment();

  killProcess(proc);
  while (1);
}

void executeProgram(char* name) {
  int x, y, programSize, procEntry, segment;
  char program[MAX_FILE_SIZE];

  programSize = getFileSize(name);

  procEntry = getFreeProcEntry();
  segment = procEntry * SEGMENT_SIZE + USER_SPACE_OFFSET;
  if (procEntry == -1) {
    setKernelDataSegment();
    printString("Error: Max number of processes reached.\n"); 
    restoreDataSegment();
    return;
  }

  readFile(name, program);

  for (x = 0; x < programSize; x += SECTOR_SIZE) {
      for(y = 0; y < SECTOR_SIZE; y++) {
        putInMemory(segment, x + y, *(program + x + y));
      }
  }
  initializeProgram(segment);
  setKernelDataSegment();
  ProcessTable[procEntry].active = 1;
  restoreDataSegment();
}

int getFreeProcEntry() {
  int x, freeProcEntry = -1;
  setKernelDataSegment();

  for (x = 0; x < MAX_PROCESSES; x++) {
    if ((ProcessTable[x].active == 0) && (ProcessTable[x].waiting == 0)) {
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
  char dirName[NAME_SIZE], fileName[NAME_SIZE];

  parseFileName(name, dirName, fileName);
  loadDirectory(directory, dirName);
  index = getDirIndex(fileName, directory);
  for (x = HEADER_SIZE; x < ENTRY_SIZE; x++) {
    if (*(directory + index + x) != 0) {
      size++;
    }
  }

  return size * SECTOR_SIZE;
}

void loadRootDirectory(char* buffer) {readSector(buffer, ROOT_DIR_SECTOR);}
void loadDirectory(char* buffer, char* name) {
  int sector;
  if (*name == '/') {
    loadRootDirectory(buffer);
  } else {
    sector = getDirSector(name);
    readSector(buffer, sector);
  }
}

void loadMap(char* buffer) {
  readSector(buffer,MAP_SECTOR);
}

void saveRootDirectory(char* buffer) {writeSector(buffer, ROOT_DIR_SECTOR);}
void saveDirectory(char* buffer, char* name) {
  int sector;
  if (*name == '/') {
    saveRootDirectory(buffer);
  } else {
    sector = getDirSector(name);
    writeSector(buffer, sector);
  }
}

void saveMap(char* buffer) {
  writeSector(buffer,MAP_SECTOR);
}

void directory(char* dirName) {
  int x, y;
  char directory[SECTOR_SIZE];

  loadDirectory(directory, dirName);

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

//returns directory index for writing data
int initEntry(char* filename, char* directory, int entryType) {
  int x, eolEncountered = 0, dirIndex;
  dirIndex = getEmptyDirIndex(directory);
  if(dirIndex == -1) {return dirIndex;}

  //write name into directory
  for (x = 0; x < NAME_SIZE; x++) {
    if ((eolEncountered) || (*(filename+x) == 0xA)) {
      eolEncountered = 1;
      *(directory + dirIndex + x) = 0;
    } else {
      *(directory + dirIndex + x) = *(filename + x);
    }
  }
  *(directory + dirIndex + TYPE_INDEX) = entryType;

  return (dirIndex + HEADER_SIZE);
}

void writeFile(char* filename, char inbuf[]) {
  int x, sector, dirIndex, eolEncountered = 0;
  char directory[SECTOR_SIZE], map[SECTOR_SIZE];
  char topName[NAME_SIZE], subName[NAME_SIZE];

  parseFileName(filename, topName, subName);
  loadDirectory(directory, topName);
  filename = subName;
  loadMap(map);
  dirIndex = initEntry(filename, directory, FILE_ENTRY);
  if (dirIndex == -1) {
    setKernelDataSegment();
    printString("ERROR: Directory is full\n"); 
    restoreDataSegment();
    return;
  }

  //write file to sectors and record in map/directory
  //size of the file is unknown, so write up to max
  //break if a sector starts as null...
  for (x = 0; x < MAX_FILE_SIZE; x += SECTOR_SIZE) {
    sector = getEmptySector(map);
    if (sector == -1) {
      setKernelDataSegment();
      printString("ERROR: Out of disk space\n"); 
      restoreDataSegment();
      return;
    }
    
    directory[dirIndex] = sector;
    dirIndex++;
    map[sector] = SECTOR_USED;

    if (*(inbuf + x) != 0) {writeSector(inbuf + x, sector);} 
    else { break;}
  }

  saveDirectory(directory, topName);
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
  char topName[NAME_SIZE], subName[NAME_SIZE];

  parseFileName(filename, topName, subName);
  loadDirectory(directory, topName);
  index = getDirIndex(subName, directory);
  if (index == -1) {
    setKernelDataSegment();
    printString("ERROR: No such file in Directory\n"); 
    restoreDataSegment();
    return;
  }

  for(x = HEADER_SIZE; x < ENTRY_SIZE; x++) {
    entryChar = directory[index + x];
    if (entryChar != 0) {
      readSector((outbuf + (x - HEADER_SIZE) * SECTOR_SIZE), (int)entryChar);
    }
  }
}

int getDirIndex(char* filename, char* directory) {
  int x, y, index, matches;
  char fileChar, entryChar;

  for (x = 0; x < SECTOR_SIZE; x += ENTRY_SIZE) {
    matches = 1;
    index = -1;

    for (y = 0; y < NAME_SIZE; y++) {
      fileChar = *(filename + y);
      entryChar = *(directory + x + y);

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

  return index;
}

void deleteFile(char* filename) {
  int y, index;
  char entryChar;
  char map[SECTOR_SIZE];
  char directory[SECTOR_SIZE];
  static char zeroSector[SECTOR_SIZE];
  char topName[NAME_SIZE], subName[NAME_SIZE];

  parseFileName(filename, topName, subName);
  loadDirectory(directory, topName);
  loadMap(map);
  index = getDirIndex(subName, directory);
//  if (index == -1) {printString("ERROR: No such file in Directory\n"); return;}

  if (index != -1) {
    for (y = 0; y < HEADER_SIZE; y++) {
      directory[index+y] = 0;
    }

    for (y = HEADER_SIZE; y < ENTRY_SIZE; y++) {
      entryChar = directory[index + y];
      if ((entryChar != 0)) {
        map[(int)entryChar] = 0;
        writeSector(zeroSector, (int)entryChar);
      }
          
      directory[index+y] = 0;
    }
    saveMap(map);
    saveDirectory(directory, topName);
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

void printInt(int n) {
  int quotient, remainder, length, x;
  int digits[INT_MAX_LENGTH];
  //432 would be stored in digits as [0,0,4,3,2]

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
      break; 
    } else {
      buffer[i] = letter;
    }
    
    printChar(letter);
  }
}
