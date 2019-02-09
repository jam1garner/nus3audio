#include "nus3audio.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

void writeAllBytes(char* filename, void* bytes, int size){
    FILE* file = fopen(filename, "wb");
    fwrite(bytes, (size_t)size, 1, file);
    fclose(file);
}

void extract(Nus3audioFile* nus, char* outfolder){
    AudioFile* current = nus->head;
    while(current){
        char* extension;
        if(memcmp(current->data, "OPUS", 4) == 0){
            extension = "lopus";
        }
        else if(memcmp(current->data, "IDSP", 4) == 0) {
            extension = "idsp";
        }
        char outFileName[256];
        sprintf((char*)&outFileName, "%s/%s.%s", outfolder, current->name, extension);
        printf("Outputting to file: %s\n", outFileName);
        writeAllBytes(outFileName, current->data, current->filesize);
        current = current->next;
    }
}

void printUsage(){
    printf("Usage:\n----------\nnus3audio [options] FILE\n\n");
    printf("  -p  --print    |  Prints info about all the files in the nus3audio archive (Default behavior)\n");
    printf("  -e  --extract  |  Extract to 'output' folder, unless another folder is provided\n");
    printf("  -w  --write    |  Write generated nus3bank to provided outpath\n");
    printf("  -o  --outpath  |  Provide an output folder name for extraction, or output file name for injection\n");
    printf("                 |  example: nus3audio -e -o mario_audio mario_sfx.nus3audio\n");
    printf("                 |\n");
    printf("  -d  --delete   |  Given a index it removes the audio file from the nus3audio file\n");
    printf("  -r  --replace  |  Replaces a file in the nus3audio file given an index and a filename\n");
    printf("                 |  example: nus3audio mario_sfx.nus3audio -r 0 mario_run.lopus -o mario_sfx_mod.nus3audio\n");
    printf("  -v  --vgmstream|  Use vgmstream compatibility mode\n");
    printf("                 |\n");
}

int main(int argc, char const *argv[])
{
    char *outpath = NULL, *filename = NULL;;
    bool actionPrint = false, actionExtract = false, actionWrite = false, vgms_compatibility = false;
    
    // Singly linked lists for storing edits to be made
    Nus3audioFile *replacementFiles = malloc(sizeof(Nus3audioFile)),
                  *deletedFiles = malloc(sizeof(Nus3audioFile));
    // Temporary variables
    AudioFile *currDeleteNode = NULL, *currentReplacementNode = NULL;
    replacementFiles->head = NULL;
    deletedFiles->head = NULL;
    replacementFiles->entryCount = 0;

    // Argument handling
    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "--help") == 0){
            printUsage();
            return 0;
        }
        else if(strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--extract") == 0){
            if(!outpath){
                outpath = "output";
            }
            actionExtract = true;
        }
        else if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--print") == 0){
            actionPrint = true;
        }
        else if(strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--write") == 0){
            if(!outpath){
                outpath = "out.nus3audio";
            }
            actionWrite = true;
        }
        else if(strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--replace") == 0){
            if(argc <= i+2){
                printf("Error: Not enough args!\n\n");
                printUsage();
                return -1;
            }
            if(!replacementFiles->head){
                replacementFiles->head = malloc(sizeof(AudioFile));
                currentReplacementNode = replacementFiles->head;
            }
            else {
                currentReplacementNode->next = malloc(sizeof(AudioFile));
                currentReplacementNode = currentReplacementNode->next;
            }
            currentReplacementNode->next = NULL;
            currentReplacementNode->id = atoi(argv[i+1]);
            FILE* rf = fopen(argv[i+2], "rb");
            fseek(rf, 0L, SEEK_END);
            long rFileSize = ftell(rf);
            fseek(rf, 0L, SEEK_SET);
            currentReplacementNode->filesize = rFileSize;
            currentReplacementNode->data = malloc(rFileSize);
            fread(currentReplacementNode->data, rFileSize, 1, rf);
            fclose(rf);
            i += 2;
        }
        else if(strcmp(argv[i], "-d") == 0){
            int indexToRemove = atoi(argv[i+1]);
            if(!deletedFiles->head){
                deletedFiles->head = malloc(sizeof(AudioFile));
                currDeleteNode = deletedFiles->head;
            }
            else {
                currDeleteNode->next = malloc(sizeof(AudioFile));
                currDeleteNode = currDeleteNode->next;
            }
            currDeleteNode->next = NULL;
            currDeleteNode->id = indexToRemove;
            i++;
        }
        else if(strcmp(argv[i], "-o") == 0){
            outpath = malloc(strlen(argv[i+1]) + 1); 
            strcpy(outpath, argv[i+1]);
            i++;
        }
        else if(strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--vgmstream") == 0){
            vgms_compatibility = true;
        }
        else {
            if(filename == NULL){
                filename = malloc(strlen(argv[i]) + 1); 
                strcpy(filename, argv[i]);
            }
        }
    }
    if(!filename){
        printUsage();
        return -1;
    }
    FILE* file = fopen(filename, "rb");
    Nus3audioFile* nus = parse_file(file);
    fclose(file);

    // Apply deletions
    currDeleteNode = deletedFiles->head;
    while(currDeleteNode){
        AudioFile* currentNode = nus->head;
        AudioFile* lastNode = NULL;
        while(currentNode){
            if(currentNode->id == currDeleteNode->id){
                if(lastNode == NULL)
                    nus->head = currentNode->next;
                else
                    lastNode->next = currentNode->next;
                free(currentNode);
                break;
            }
            lastNode = currentNode;
            currentNode = currentNode->next;
        }
        currDeleteNode = currDeleteNode->next;
    }

    // Apply replacements
    currentReplacementNode = replacementFiles->head;
    while(currentReplacementNode){
        AudioFile* currentNode = nus->head;
        while(currentNode){
            if(currentNode->id == currentReplacementNode->id){
                currentNode->data = currentReplacementNode->data;
                currentNode->filesize = currentReplacementNode->filesize;
                break;
            }
            currentNode = currentNode->next;
        }
        currentReplacementNode = currentReplacementNode->next;
    }

    if(actionPrint || !(actionExtract || actionWrite)){
        AudioFile* currentNode = nus->head;
        while(currentNode){
            printf("%.2i: filesize = %.8X, name = %s\n", currentNode->id, currentNode->filesize, currentNode->name);
            currentNode = currentNode->next;
        }
    }
    
    if(actionWrite){
        FILE* f = fopen(outpath, "wb");
        write_file(f, nus, vgms_compatibility);
        fclose(f);
    }
    else if(actionExtract){
        char command[256];
        snprintf((char*)&command, 256, "mkdir \"%s\"", outpath);
        system(command);
        extract(nus, outpath);
    }

    return 0;
}
