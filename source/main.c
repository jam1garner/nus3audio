#include "nus3audio.h"
#include <string.h>
#include <stdlib.h>

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

int main(int argc, char const *argv[])
{
    char* outfolder = "output";
    char* filename = NULL;
    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "--help") == 0){
            printf("Usage:\n----------\nnus3audio [-o (output)] FILE\n");
            return 0;
        }
        else if(strcmp(argv[i], "-o") == 0){
            outfolder = malloc(strlen(argv[i+1]) + 1); 
            strcpy(outfolder, argv[i+1]);
            i++;
        }
        else {
            if(filename == NULL){
                filename = malloc(strlen(argv[i]) + 1); 
                strcpy(filename, argv[i]);
            }
        }
    }
    if(!filename){
        printf("No filename given.\n\n");
        printf("Usage:\n----------\nnus3audio [-o (output folder)] FILE\n");
        return -1;
    }

    char command[256];
    sprintf((char*)&command, "mkdir \"%s\"", outfolder);
    system(command);

    FILE* file = fopen(filename, "rb");
    Nus3audioFile* nus = parse_file(file);
    extract(nus, outfolder);
    fclose(file);

    return 0;
}
