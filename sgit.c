#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_BOLD          "\x1b[1m"
#define ANSI_ERROR         "\x1b[1;31m"
//*note, can combine multiple codes, ex. red + bold \x1b[31;1m

void help();
void init(char* dir);
void commit(char* dir, char* msg);
char* readLine(FILE* fptr);
char** constructFile(char* sgitpath, char* filePath, int* numberOfLines, int commitNum);
char* generateInitialCommitString(char* sgittxtPath, char* fileRelativePath);
char* commitFile(char* sgitPath, char* sgitDirPath, char* fileRelativePath);
char* stripNewLine(char* str);
void checkout(int checkoutNum, char* dir);

int main(int numArgs, char* args[]) {
    printf(ANSI_COLOR_RESET);
    switch (numArgs) {
        case 1:
            help();
            /*
            int numLines = 0;
            char** lines = constructFile("./project/.sgit.txt", "2ndfile.txt", &numLines, -1);
            //printf("returned %d lines\n", numLines);
            for (int i = 0; i < numLines; i++) {
                //printf("ran\n");
                printf(lines[i]);
            }
            break;
            */
        case 2:
            if (strcmp(args[1], "help") == 0) {
                help();
            }
            break;
        case 3:
            if (strcmp(args[1], "init") == 0) {
                init(args[2]);
            } 
            break;
        case 4:
            if (strcmp(args[1], "commit") == 0) {
                commit(args[2], args[3]);
            } else if (strcmp(args[1], "checkout") == 0) {
                checkout(atoi(args[3]), args[2]);
            }
            break;
        default:
            printf("\x1b[1;31mInvalid arguments" ANSI_COLOR_RESET);
    }
    
}

// MAIN FUNCTIONS
void commit(char* dir, char* msg) {
    char* fullDir = malloc(strlen(dir)+11);
    strcpy(fullDir, dir);
    strcat(fullDir,".sgit.txt");
    FILE* fptr = fopen(fullDir, "r");

    if (fptr != NULL) {
        char* line = readLine(fptr);
        free(line);
        line = readLine(fptr);
        if (strcmp(line, ">") == 0) {
            //this is the initial commit
            printf("Creating Initial Commit...\n");
            char* toAppend = malloc(15);
            strcpy(toAppend, ">> initcommit\n");
            
            char* winSearchPath = malloc(strlen(dir)+2);
            strcpy(winSearchPath, dir);
            strcat(winSearchPath, "*");
            WIN32_FIND_DATA findData;   //CHANGE LATER
            HANDLE hFind = FindFirstFile(winSearchPath, &findData);

            if (hFind == INVALID_HANDLE_VALUE) {
                printf("Couldn't open directory");
                return;
            }

            do {
                printf("found %s\n", findData.cFileName);
                if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0 && strcmp(findData.cFileName, ".sgit.txt") != 0) {
                    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        //directory
                        //ill deal with this later, probably should do recursion or something
                    } else {
                        //file
                        
                        char* name = findData.cFileName;
                        char* toAdd = generateInitialCommitString(dir, name);
                        toAppend = realloc(toAppend, strlen(toAppend) + strlen(toAdd) + 1);
                        strcat(toAppend, toAdd);
                        free(toAdd);
                    }
                }
            } while (FindNextFile(hFind, &findData) != 0);
            
            FindClose(hFind);


            toAppend = realloc(toAppend, strlen(toAppend) + 2);
            strcat(toAppend, ">");
            printf("Adding to new repo:\n%s\n---", toAppend);

            fclose(fptr);
            fptr = fopen(fullDir, "a");
            fprintf(fptr, toAppend);
            fclose(fptr);
            return;
            
        } else {
            //navigate to end of document
            int numCommits = 0;
            //printf("navigating to end of file, curr line: %s\n", line);
            while (line[0] != '>' || line[1] != '\0') {
                char* prefixCurrLine = malloc(11);
                strncpy(prefixCurrLine, line, 10);
                prefixCurrLine[10] = '\0';
                //printf("line: %s\nprefix: %s\n", line, prefixCurrLine);
                if (strcmp(prefixCurrLine, ">>> commit") == 0) {
                    numCommits++;
                }

                free(line);
                line = readLine(fptr);
            }
            printf("commit #: %d\n", numCommits);
            char* winSearchPath = malloc(strlen(dir)+2);
            strcpy(winSearchPath, dir);
            strcat(winSearchPath, "*");
            WIN32_FIND_DATA findData;   //CHANGE LATER
            HANDLE hFind = FindFirstFile(winSearchPath, &findData);

            if (hFind == INVALID_HANDLE_VALUE) {
                printf("Couldn't open directory");
                return;
            }
            
            char numCommitsStr[12];
            snprintf(numCommitsStr, sizeof(numCommitsStr), "%d", numCommits);
            char* toAppend = malloc(13 + strlen(numCommitsStr) + strlen(msg));
            strcpy(toAppend, ">> commit ");
            strcat(toAppend, numCommitsStr);
            strcat(toAppend, " ");
            strcat(toAppend, msg);
            strcat(toAppend, "\n");
            do {
                if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0 && strcmp(findData.cFileName, ".sgit.txt") != 0) {
                    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        //directory
                        //ill deal with this later, probably should do recursion or something
                    } else {
                        //file    
                        char* name = findData.cFileName;
                        char* diffToAdd = commitFile(fullDir, dir, name);
                        //printf("adding diff: %s\n---\n", diffToAdd);
                        toAppend = realloc(toAppend, strlen(toAppend) + strlen(diffToAdd) + 1);
                        strcat(toAppend, diffToAdd);
                        free(diffToAdd);
                    }
                }
            } while (FindNextFile(hFind, &findData) != 0);
            toAppend = realloc(toAppend, strlen(toAppend) + 2);
            strcat(toAppend, ">");
            
            fclose(fptr);
            fptr = fopen(fullDir, "a");
            fprintf(fptr, toAppend);
            fclose(fptr);
            return;
        }
        

    } else {
        printf(ANSI_ERROR "Cannot commit, no repo initialized at %s" ANSI_COLOR_RESET, fullDir);
    }
}

void init(char* dir) {
    char* fullDir = strcat(dir,"/.sgit.txt");
    FILE* fptr = fopen(fullDir, "r");

    if (fptr == NULL) {
        fclose(fptr);
        printf("Creating new repo...");
        fptr = fopen(fullDir, "w");
        fprintf(fptr, ">>> init repo\n>");
        fclose(fptr);
    } else {
        printf("Repo already exists");
        fclose(fptr);
    }
    return;
}

void help() {
    printf(ANSI_BOLD "Commands: sgit" ANSI_COLOR_RESET "\n\thelp - show this message\n\tinit DIRECTORY - initialize repository in path specified, relative to current directory\n\tcommit DIRECTORY MESSAGE - Save current files to repo\n\tcheckout DIRECTORY COMMIT_NUM - Revert to previous commit, deleting current state");
}

void checkout(int checkoutNum, char* dir) {
    char* winSearchPath = malloc(strlen(dir)+2);
    strcpy(winSearchPath, dir);
    strcat(winSearchPath, "*");
    WIN32_FIND_DATA findData;   //CHANGE LATER
    HANDLE hFind = FindFirstFile(winSearchPath, &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Couldn't open directory");
        return;
    }

    do {
        //printf("found %s\n", findData.cFileName);
        if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0 && strcmp(findData.cFileName, ".sgit.txt") != 0) {
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                //directory
                //ill deal with this later, probably should do recursion or something
            } else {
                //file
                char* name = findData.cFileName;
                char* filePath = malloc(strlen(name) + strlen(dir) + 1);
                strcpy(filePath, dir);
                strcat(filePath, name);
                if (remove(filePath) != 0) {
                    printf(ANSI_ERROR "Unable to delete %s\n" ANSI_COLOR_RESET, filePath);
                }
            }
        }
    } while (FindNextFile(hFind, &findData) != 0);
    FindClose(hFind);

    char* sgitPath = malloc(strlen(dir)+10);
    strcpy(sgitPath, dir);
    strcat(sgitPath, ".sgit.txt");
    FILE* fptr = fopen(sgitPath, "r");
    char* line = readLine(fptr);
    while (line != NULL) {
        if (strncmp(line, "$$", 2) == 0){
            int nameEndIdx = (strchr(&line[2], '\n') - &line[2])-1;
            char* tmp = malloc(nameEndIdx);
            strcpy(tmp, &line[2]);

            char* fileName = stripNewLine(tmp);
            int numLines = 0;
            
            char** constructedFile = constructFile(sgitPath, fileName, &numLines, checkoutNum);
            
            char* newFilePath = malloc(strlen(dir) + strlen(fileName) + 1);
            strcpy(newFilePath, dir);
            strcat(newFilePath, fileName);
            printf("writing %s\n", newFilePath);
            FILE* newFile = fopen(newFilePath, "w");
            for (int i = 0; i < numLines; i++) {
                fprintf(newFile, constructedFile[i]);
            }

            fclose(newFile);
            free(tmp);
            free(fileName);
        }
        free(line);
        line = readLine(fptr);
    }
}

//HELPERS

int intFromStringWithDelimiter(char* strStart, char delimiter) {
    int lineNumIdx = (strchr(strStart, delimiter) - strStart)-1;
    char* lineNumStr = malloc((lineNumIdx+2)*sizeof(char*));
    strncpy(lineNumStr, strStart, (size_t) lineNumIdx+1);
    lineNumStr[lineNumIdx+1] = '\0';
    int lineNum; //number read from string
    lineNum = atoi(lineNumStr);
    //printf("line num str: %s\n", lineNumStr);
    //printf("line num: %d\n", lineNum);
    free(lineNumStr);
    return lineNum;
}
//return string representation of the file passed in, ready to be inserted into the .sgit.txt file
                // ex. ./project/   | ex. src/file.txt -> file being converted is at ./project/src/file.txt
char* generateInitialCommitString(char* sgitdirPath, char* fileRelativePath) {
    char* str = malloc(strlen(str) + strlen(fileRelativePath) + 4);
    strcpy(str,"$$");
    strcpy(&str[2], fileRelativePath);
    strcpy(&str[strlen(str)], "\n");

    char* path = malloc(strlen(sgitdirPath) + strlen(fileRelativePath) + 1);
    strcpy(path, sgitdirPath);
    strcpy(&path[strlen(path)], fileRelativePath);
    //printf("PATH USED: %s\n", path);
    FILE* userfptr = fopen(path, "r");
    char* userFileLine = readLine(userfptr);
    int currLen = strlen(str);
    while (userFileLine != NULL) {
        //printf("inside while loop");
        currLen = strlen(str);
        //printf("got length");
        str = realloc(str, currLen + strlen(userFileLine) + 1);
        //printf("reallocated");
        strcat(str, userFileLine);
        free(userFileLine);
        
        userFileLine = readLine(userfptr);
        //printf(userFileLine);
        //printf("char 0 is: %c\n", userFileLine[0]);
        
    }
    currLen = strlen(str);
    str = realloc(str, strlen(str) + 4);
    strcat(str, "\n$\n");
    fclose(userfptr);
    printf("generated new file:\n%s\n---\n", str);
    return str;
}

//sgitPath: path to .sgit.txt file, filePath: path of file to be committed
// returns string of diffs to be added to sgit.txt file
char* commitFile(char* sgitPath, char* sgitDirPath, char* fileRelativePath) {
    int numLines = 0;
    //printf("constructing with sgit path: %s, file path: %s...\n", sgitPath, fileRelativePath);
    char** fileFromsGit = constructFile(sgitPath, fileRelativePath, &numLines, -1);
    //printf("constructed: %s", fileFromsGit[0]);

    if (fileFromsGit == NULL) {
        //new file
        printf("New File, Generating initial commit...\n");
        char* toAdd = generateInitialCommitString(sgitDirPath, fileRelativePath);
        return toAdd;
    } else {
        //calculate diffs
        
        char* toAdd = malloc(strlen(fileRelativePath) + 4);
        strcpy(toAdd, ">>");
        strcat(toAdd, fileRelativePath);
        strcat(toAdd, "\n");

        char* newFilePath = malloc(strlen(sgitDirPath) + strlen(fileRelativePath) + 1);
        strcpy(newFilePath, sgitDirPath);
        strcat(newFilePath, fileRelativePath);
        printf("gettings diffs of %s\n", newFilePath);
        FILE* newFile = fopen(newFilePath, "r");
        free(newFilePath);

        int oldIdx = 1;
        char* prevLine = readLine(newFile);
        char* currLine = readLine(newFile);
        int newFileLineNum = 0;

        while (oldIdx <= numLines || prevLine != NULL) {
            //printf("reading line...\n");
            char lineNumStr[12];
            snprintf(lineNumStr, sizeof(lineNumStr), "%d", oldIdx-1);
            
            
            if (prevLine == NULL) {
                //printf(ANSI_COLOR_YELLOW "old is longer\n" ANSI_COLOR_RESET);
                char* diffToAdd = malloc(strlen(lineNumStr) + 4);
                strcpy(diffToAdd, lineNumStr);
                strcat(diffToAdd, ";-\n");
                
                toAdd = realloc(toAdd, strlen(toAdd) + strlen(diffToAdd) + 1);
                strcat(toAdd, diffToAdd);
                free(diffToAdd);
                oldIdx++;
            } else if (oldIdx > numLines) {
                //printf(ANSI_COLOR_YELLOW "new is longer\n" ANSI_COLOR_RESET);

                //char newFileLineNumStr[12];
                //snprintf(newFileLineNumStr, sizeof(newFileLineNumStr), "%d", newFileLineNum);

                char* diffToAdd = malloc(strlen(lineNumStr) + strlen(prevLine) + 3);
                strcpy(diffToAdd, lineNumStr);
                strcat(diffToAdd, ";+");
                strcat(diffToAdd, prevLine);

                toAdd = realloc(toAdd, strlen(toAdd) + strlen(diffToAdd) + 1);
                strcat(toAdd, diffToAdd);
                free(diffToAdd);

                newFileLineNum++;
                free(prevLine);
                prevLine = currLine;
                if (currLine != NULL) {
                    currLine = readLine(newFile);
                } 
            } else {
                char* cmpOldLine = stripNewLine(fileFromsGit[oldIdx-1]);
                char* cmpNewLine = stripNewLine(prevLine);
                char* cmpNextOldLine;
                if (oldIdx < numLines) {
                    cmpNextOldLine = stripNewLine(fileFromsGit[oldIdx]);
                } else {
                    cmpNextOldLine = NULL; 
                }
                char* cmpNextNewLine = stripNewLine(currLine);
                
                
                //printf("old: %s, new: %s\n", cmpOldLine, cmpNewLine);
                if (strcmp(cmpOldLine, cmpNewLine) != 0) {
                    //printf("OLD: %s, NEW: %s\n", cmpOldLine, cmpNewLine);
                    //printf(ANSI_COLOR_GREEN "found diff: " ANSI_COLOR_RESET);
                    if (prevLine[strlen(prevLine)-1] != '\n') {
                        prevLine = realloc(prevLine, strlen(prevLine) + 2);
                        strcat(prevLine, "\n");
                        //printf(ANSI_COLOR_YELLOW "prevLIne: %s" ANSI_COLOR_RESET, prevLine);
                    }
                    if (oldIdx < numLines && strcmp(cmpNextOldLine, cmpNewLine) == 0) {
                        
                        //deletion
                        //printf("deletion\n");
                        char* diffToAdd = malloc(strlen(lineNumStr) + 3);
                        strcpy(diffToAdd, lineNumStr);
                        strcat(diffToAdd, ";-");
                        
                        toAdd = realloc(toAdd, strlen(toAdd) + strlen(diffToAdd) + 1);
                        strcat(toAdd, diffToAdd);
                        free(diffToAdd);

                        free(prevLine);
                        prevLine = currLine;
                        currLine = readLine(newFile);
                        
                        oldIdx++;   
                        
                    } 
                    if (currLine != NULL && strcmp(cmpOldLine, cmpNextNewLine) == 0) {
                        //insertion
                        //printf("insertion\n");
                        char* diffToAdd = malloc(strlen(lineNumStr) + strlen(prevLine) + 3);
                        strcpy(diffToAdd, lineNumStr);
                        strcat(diffToAdd, ";+");
                        strcat(diffToAdd, prevLine);
                        
                        //printf(ANSI_COLOR_BLUE "%s" ANSI_COLOR_RESET, diffToAdd);
                        toAdd = realloc(toAdd, strlen(toAdd) + strlen(diffToAdd) + 1);
                        strcat(toAdd, diffToAdd);
                        free(diffToAdd);

                        newFileLineNum++;
                        free(prevLine);
                        prevLine = currLine;
                        if (currLine != NULL) {
                            currLine = readLine(newFile);
                        }
                        
                    } else {
                        //single line change
                        //printf("replace\n");
                        char* diffToAdd = malloc(2*strlen(lineNumStr) + strlen(prevLine) + 6);
                        strcpy(diffToAdd, lineNumStr);
                        strcat(diffToAdd, ";+");
                        strcat(diffToAdd, prevLine);
                        strcat(diffToAdd, lineNumStr);
                        strcat(diffToAdd, ";-\n");

                        toAdd = realloc(toAdd, strlen(toAdd) + strlen(diffToAdd) + 1);
                        strcat(toAdd, diffToAdd);
                        free(diffToAdd);

                        oldIdx++;
                        newFileLineNum++;
                        free(prevLine);
                        prevLine = currLine;
                        if (currLine != NULL) {
                            currLine = readLine(newFile);
                        }
                    }
                } else {
                    oldIdx++;
                    newFileLineNum++;
                    free(prevLine);
                    prevLine = currLine;
                    if (currLine != NULL) {
                        currLine = readLine(newFile);
                    }
                }
                free(cmpOldLine);
                free(cmpNewLine);
                free(cmpNextOldLine);
                free(cmpNextNewLine);
            }
        }
        if (toAdd[strlen(toAdd)-1] != '\n') {
            toAdd = realloc(toAdd, strlen(toAdd) + 4);
            strcat(toAdd, "\n");
        } else {
            toAdd = realloc(toAdd, strlen(toAdd) + 3);
        }
        strcat(toAdd, "<\n");
        //printf("generated diffs:\n%s---\n", toAdd);
        return toAdd;
    }
}

//returns new copy of str with trailing \n removed
char*  stripNewLine(char* str) {
    if (str != NULL) {
        char* newStr = malloc(strlen(str)+1);
        strcpy(newStr, str);
        if (newStr[strlen(newStr)-1] == '\n') {
            newStr[strlen(newStr)-1] = '\0';
        }
        return newStr;
    } else {
        return NULL;
    }

}

//assumes sgit file is valid            relative to sgit directory      commit to construct up to, -1 for all
//NOTE applies changes in reverse order
char** constructFile(char* sgitPath, char* fileName, int* numberOfLines, int commitNum) {
    FILE* sgitfptr = fopen(sgitPath, "r");

    char** constructedFile = malloc(sizeof(char*));
    int numLines = 0;

    //create initial file

    char* currLine = readLine(sgitfptr);
    int pathLen = strlen(fileName);
    char* initKey = malloc(4 + pathLen);
    strcpy(initKey, "$$");
    strcpy(&initKey[2], fileName);
    strcpy(&initKey[strlen(initKey)], "\n");
    while (currLine != NULL) {
        if (strcmp(currLine, initKey) == 0) {
            //found initial file description
            break;
        }
        free(currLine);
        currLine = readLine(sgitfptr);
    }



    if (currLine == NULL) {
        printf(ANSI_ERROR "File(%s) not found in .sgit.txt" ANSI_COLOR_RESET, fileName);
        free(constructedFile);
        return NULL;
    } else {
        free(currLine);
        currLine = readLine(sgitfptr);       
        while (strcmp(currLine, "$\n") != 0){
            constructedFile = realloc(constructedFile, sizeof(char*)*(numLines+1));
            constructedFile[numLines] = currLine;
            currLine = readLine(sgitfptr);
            //printf("\t%s", constructedFile[numLines]);
            numLines++;
        }
    }


    //add diffs

    char* commitKey = malloc(pathLen+4);
    strcpy(commitKey, ">>");
    strcpy(&commitKey[2], fileName);
    strcpy(&commitKey[strlen(commitKey)], "\n");

    free(currLine);
    currLine = readLine(sgitfptr);

    int commitNumTracker;
    while (currLine != NULL) {
        if (strncmp(currLine, ">>> commit", 10) == 0) {
            commitNumTracker = intFromStringWithDelimiter(&currLine[11], ' ');
            printf("at commit number %d, commit num: %d\n", commitNumTracker, commitNum);
        }
        if ((commitNum < 0 || commitNumTracker <= commitNum) && strcmp(commitKey, currLine) == 0) {
            //printf("%d || %d\n", (commitNum < 0), commitNumTracker <= commitNum);
            //printf("\t%d <= %d?\n", commitNumTracker, commitNum);
            int numDiffs = 0;
            char** diffs = malloc(sizeof(char*));
            free(currLine);
            currLine = readLine(sgitfptr);
            while (strcmp(currLine, "<\n") != 0 && strcmp(currLine, "<") != 0) {
                diffs = realloc(diffs, sizeof(char*)*(numDiffs+1));
                diffs[numDiffs] = currLine;
                numDiffs++;
                currLine = readLine(sgitfptr);
            }
            //printf("num diffs: %d\n", numDiffs);
            for (int i = numDiffs-1; i >= 0; i--) {
                //printf("handling diff: %s\n", diffs[i]);
                //printf("getting int from string\n");
                int lineNum = intFromStringWithDelimiter(diffs[i],';');
                //printf("result was: %d\n", lineNum);

                int lineNumIdx = (strchr(diffs[i], ';') - diffs[i])-1;
                
                char opChar = diffs[i][lineNumIdx+2];
                
                if (opChar == '-') {
                    //printf("num lines left: %d\n", (numLines-lineNum-1));
                    if (numLines-lineNum-1 > 0) {
                        
                        memmove(&constructedFile[lineNum], &constructedFile[lineNum+1], (numLines-lineNum-1)*sizeof(char*));
                    }
                    //printf("memmoved");
                    numLines--;
                    constructedFile = realloc(constructedFile, sizeof(char*)*numLines);
                    //printf("string at line %d is now %s", lineNum, constructedFile[lineNum]);
                    //printf("remove successful");
                } else if (opChar == '+') {
                    int lenOfAddition = strlen(diffs[i])-lineNumIdx-1;

                    constructedFile = realloc(constructedFile, sizeof(char*)*(numLines+1));
                    if (lineNum < numLines){
                        memmove(&constructedFile[lineNum+1], &constructedFile[lineNum], (numLines-lineNum)*sizeof(char*));
                    }
                    
                    numLines++;
                    if (lenOfAddition > 0) {
                        char* restOfLine = malloc(lenOfAddition);
                        strncpy(restOfLine, &diffs[i][lineNumIdx+3], lenOfAddition);
                        restOfLine[lenOfAddition-1] = '\0';
                        constructedFile[lineNum] = restOfLine;
                    } else {
                        printf(ANSI_ERROR "Attempted to add line with length 0" ANSI_COLOR_RESET);
                    }
                }
            }
            free(diffs);
        }
        free(currLine);
        currLine = readLine(sgitfptr);
    }
    free(commitKey);

    constructedFile[numLines-1][strlen(constructedFile[numLines-1])-1] = '\0';
    *numberOfLines = numLines;
    return constructedFile;

}

//reads and returns a pointer to the next line in a file, returns empty string if the line is too large (~186 ish chars)
char* readLine(FILE* fptr) {
    int currBuffSize = 32;
    char* wholeString = malloc(sizeof(char)*32);
    char buff[32];
    if (fgets(buff, 32, fptr) == NULL) {
        free(wholeString);
        return NULL;
    }
    
    strcpy(wholeString, buff);
    //printf("%d\n", (int) strlen(buff));
    int i = 0;
    while (strlen(buff) == 31 && buff[30] != '\n'){
        //overflow   //pretty sure its just 1, but whatever | increase by 31 because first char of next section overwrites previous \0 
        wholeString = realloc(wholeString, sizeof(char)*(currBuffSize+31));
        if (fgets(buff, 32, fptr) == NULL) {
            free(wholeString);
            return NULL;
        }
        strcpy(&wholeString[currBuffSize-1], buff);
        currBuffSize += 31;
        i ++;
        //printf("%d\n", i);
        if (i > 5) {
            printf("REALLY BIG LINE, TERMINATING");
            free(wholeString);
            return "";
        }
    }
    return wholeString;
}