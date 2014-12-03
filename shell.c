#define SCREEN_WIDTH 80
#define MAX_COMMAND_LENGTH 10
#define MAX_FILE_SIZE 13312 
#define NAME_LENGTH 6
#define MAX_PATH_LEN 100

int interrupt (int number, int AX, int BX, int CX, int DX);
void print(char* str);
void readLine(char* buffer);
void directory(char* name);
void printFile(char* name);
void deleteFile(char* name);
void createFile(char* name);
void readFile(char* name, char* buffer);
void writeFile(char* name, char* buffer);
void execFile(char* name);
void shellExec();
int parseCommand(char* buffer);
int compareCommand(char* expectedCmd, char* str);
void kill(int proc);
int argLength(char* arg);
int stringToInt(char* arg);
int power(int base, int exp);
void printMessage();
void enableInterrupts();
void changeDirectory(char* path);
void makeDirectory(char* name);
void constructPath(char* name, char* argumentBuffer);

void main() {
  enableInterrupts();

  while (1) {
    shellExec();
  }
}
char WorkingDir[MAX_PATH_LEN] = {'/'};

void shellExec() {
  int x, offset;
  char wd[MAX_PATH_LEN];
  char command[MAX_COMMAND_LENGTH];
  char inputBuffer[SCREEN_WIDTH];
  char fileBuffer[MAX_FILE_SIZE];
  char arg[SCREEN_WIDTH - MAX_COMMAND_LENGTH];
  char argument[MAX_PATH_LEN];
  int cmdEndIndex = 0;

  print("sh "); print(WorkingDir); print("> ");
  readLine(inputBuffer);
  cmdEndIndex = parseCommand(inputBuffer);
  for (x = 0; x <= cmdEndIndex; x++) {
    command[x] =  inputBuffer[x];
    if (x == cmdEndIndex) {
      command[x + 1] = 0; 
    }
  }

  offset = cmdEndIndex + 2;
  for (x = offset; x <= SCREEN_WIDTH; x++) {
    arg[x - offset] = inputBuffer[x];
    if (inputBuffer[x] == 0xA) {break;} //break at line feed
  }

  constructPath(arg, argument);
  if (compareCommand("dir",command)) {
    directory(WorkingDir);
  } else if (compareCommand("type",command)) {
    printFile(argument);
  } else if (compareCommand("del",command)) {
    deleteFile(argument);
  } else if (compareCommand("create",command)) {
    createFile(argument);
  } else if (compareCommand("execute",command)) {
    execFile(argument);
  } else if (compareCommand("kill",command)) {
    kill(stringToInt(argument));
  } else if (compareCommand("cd",command)) {
    changeDirectory(argument);
  } else if (compareCommand("mkdir",command)) {
    makeDirectory(argument);
  } else {
    print("Invalid command!\n");
  }
}

//supply this function with the name of the file
//and a buffer for the argument
void constructPath(char* name, char* argumentBuffer) {
  int i = 0, x = 0, insertSlash = 0;
  
  if ((*name == '/' && *(name + 1) == 0xA) || (*name == '.' && *(name + 1) == '.')) {
    *argumentBuffer = '/';
    *(argumentBuffer + 1)= 0xA;
    return;
  }

  for(i = 0; i < MAX_PATH_LEN; i++) {
    if(WorkingDir[i] != 0 && WorkingDir[i] != 0xA && WorkingDir[i] != '/') {
      *(argumentBuffer + i) = WorkingDir[i];
      insertSlash = 1;
    } else if (*(name + x) != 0 && *(name + x) != 0xA) {
      if(insertSlash) {
        *(argumentBuffer + i) = '/';
        insertSlash = 0;
      } else {
        *(argumentBuffer + i) = *(name + x);
        x++;
      }
    } 
  }
}
void changeDirectory(char* path) {
  int i = 0, x = 0, lastSlash = 0;
  char c;

  if ((*path == '/' && *(path + 1) == 0xA) || (*path == '.' && *(path + 1) == '.')) {
    *WorkingDir = '/';
  } else {
    c = *path;
    if (c == '/') {i++;}
    if (c == 0) {return;}
    for(x = i; i < MAX_PATH_LEN; x++) {
      c = *(path + x);
      if (c != 0 && c != 0xA) {
        WorkingDir[x] = c;
        if(c == '/') {lastSlash = x;}
      } else {
        break;
      }
    }
  }

  //clean up wd path
  if(lastSlash > 0) {
    i = lastSlash;
  } else {
    i = x + 1;
  }
  for(x = i; x < MAX_PATH_LEN; x++) {
    WorkingDir[x] = 0;
  }
}

//returns ending index of command
int parseCommand(char* buffer) {
  int x = 0;
  int endIndex = -1;
  char bufferChar = *buffer;

  while ((bufferChar != ' ') && (bufferChar != 0) && (bufferChar != 0xA)) { 
    endIndex = x;
    x++;
    bufferChar = *(buffer + x);
  }

  return endIndex;
}

int compareCommand(char* expectedCmd, char* str) {
  int x = 0;
  int result = 1;
  char expectedChar, strChar;
  
  do {
    expectedChar = *(expectedCmd + x);
    strChar = *(str + x);
    x++;

    if (strChar != expectedChar){
      result = 0;
      break;
    }
  } while (strChar != 0);
  return result;
}

//string must end with 0xA, see argLength
int stringToInt(char* arg) {
  int result = 0, i, length = argLength(arg);
  for (i = length; i > 0; i--) {
    result += power(*(arg + (length - i)) - 48, i);
  }
  return result;
}

int power(int base, int exp) {
  int i, result = 1;
  for (i = 0; i < exp; i++) {
    result *= base;
  }
  return result;
}

int argLength(char* arg) {
  int x = 0;
  while (0xA != *(arg + x)) {
    x++;
  }
  return x;
}

void makeDirectory(char* name) {
  interrupt(0x21,11,(int)name,0,0);
}

void kill(int proc) {
  interrupt(0x21,10,proc,0,0);
}

void print(char* str) {
  interrupt(0x21,0,(int)str,0,0);
}

void readLine(char* buffer) {
  interrupt(0x21,1,(int)buffer,0,0);
}

void directory(char* wd) {
  interrupt(0x21,3,(int)wd,0,0);
}

void printFile(char* name) {
  char buffer[MAX_FILE_SIZE];
  int i;

  for(i = 0; i < MAX_FILE_SIZE; i++) {buffer[i] = 0;}
  readFile(name, buffer);
  print(buffer);
}

void deleteFile(char* name) {
  interrupt(0x21,4,(int)name,0,0);
}

void createFile(char* name) {
  int x;
  int index = 0;
  char inputBuffer[MAX_FILE_SIZE];
  char lineBuffer[SCREEN_WIDTH];

  while (1) {
    readLine(lineBuffer);

    if (lineBuffer[0] == 0xA) {
      break;
    }

    for(x = 0; x < SCREEN_WIDTH; x++) {
      inputBuffer[index] = lineBuffer[x];
      lineBuffer[x] = 0;
      index++;
      if (lineBuffer[x] == 0xA) {break;}
    }
  }
  writeFile(name, inputBuffer);
}

void execFile(char* name) {
  interrupt(0x21,9,(int)name,0,0);
}

void readFile(char* name, char* buffer) {
  interrupt(0x21,6,(int)name,(int)buffer,0);
}

void writeFile(char* name, char* buffer) {
  interrupt(0x21,8,(int)name,(int)buffer,0);
}
