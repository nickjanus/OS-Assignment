#define SCREEN_WIDTH 80
#define MAX_COMMAND_LENGTH 10
#define MAX_FILE_SIZE 13312 

int interrupt (int number, int AX, int BX, int CX, int DX);
void print(char* str);
void readLine(char* buffer);
void directory();
void printFile(char* name);
void deleteFile(char* name);
void createFile(char* name);
void readFile(char* name, char* buffer);
void writeFile(char* name, char* buffer);
void execFile(char* name);
void close();
int parseCommand(char* buffer);
int compareCommand(char* expectedCmd, char* str);

void main() {
  int x, offset;
  char command[MAX_COMMAND_LENGTH];
  char inputBuffer[SCREEN_WIDTH];
  char fileBuffer[MAX_FILE_SIZE];
  char argument[SCREEN_WIDTH - MAX_COMMAND_LENGTH];
  int cmdEndIndex = 0;

  while (1) {
    print("sh> ");
    readLine(inputBuffer);
    cmdEndIndex = parseCommand(inputBuffer);
print("2");
    for (x = 0; x <= cmdEndIndex; x++) {
      command[x] =  inputBuffer[x];
      if (x == cmdEndIndex) {
        command[x + 1] = 0; 
      }
    }

print("3");
    offset = cmdEndIndex + 2;
    for (x = offset; x <= SCREEN_WIDTH; x++) {
      argument[x - offset] = inputBuffer[x];
      if (inputBuffer[x] == 0) {break;}
    }

print("4");
print(command);
    if (compareCommand("dir",command)) {
print("5");
      directory();
    } else if (compareCommand("type",command)) {
      printFile(argument);
    } else if (compareCommand("del",command)) {
      deleteFile(argument);
    } else if (compareCommand("create",command)) {
      createFile(argument);
    } else if (compareCommand("execute",command)) {
      execFile(argument);
    } else {
      print("Invalid command!");
    }

print("end");
  }
}

//returns ending index of command
int parseCommand(char* buffer) {
  int x = 0;
  int endIndex = -1;
  char bufferChar = *buffer;

  while ((bufferChar != ' ') && (bufferChar != 0)) { 
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
  
print("a");
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

void print(char* str) {
  interrupt(0x21,0,(int)str,0,0);
}

void readLine(char* buffer) {
  interrupt(0x21,1,(int)buffer,0,0);
}

void directory() {
print("directory();");
  interrupt(0x21,3,0,0,0);
}

void printFile(char* name) {
  char buffer[MAX_FILE_SIZE];
  readFile(name, buffer);
  print(buffer);
}

void deleteFile(char* name) {
  interrupt(0x21,4,(int)name,0,0);
}

void createFile(char* name) {
  int x;
  int empty = 0;
  int index = 0;
  char inputBuffer[MAX_FILE_SIZE];
  char lineBuffer[SCREEN_WIDTH];

  while (!empty) {
    readLine(lineBuffer);

    empty = 1;
    for(x = 0; x < SCREEN_WIDTH; x++) {
      if (lineBuffer[x] != 0) {
        empty = 0;
        break;
      }
    }

    if (!empty) {
      for(x = 0; x < SCREEN_WIDTH; x++) {
        inputBuffer[index] = lineBuffer[x];
        lineBuffer[x] = 0;
        index++;
      }
    }
  }
  writeFile(name, inputBuffer);
}

void execFile(char* name) {
  interrupt(0x21,9,(int)name,0x2000,0);
}

void readFile(char* name, char* buffer) {
  interrupt(0x21,6,(int)name,(int)buffer,0);
}

void writeFile(char* name, char* buffer) {
  interrupt(0x21,8,(int)name,(int)buffer,0);
}
