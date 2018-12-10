#include "nus3audio.h"
#include <string.h>

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
    if(argc < 2)
        return -1;
    FILE* file = fopen(argv[1], "rb");
    Nus3audioFile* nus = parse_file(file);
    extract(nus, "output");
    fclose(file);

    return 0;
}
