#include "nus3audio.h"

int main(int argc, char const *argv[])
{
    if(argc < 2)
        return -1;
    FILE* file = fopen(argv[1], "rb");
    Nus3audioFile* nus = parse_file(file);
    AudioFile* current = nus->head;
    while(current){
        printf("------------------------------------\n");
        printf("Name - %s\n", current->name);
        printf("ID - %i\n", current->id);
        printf("Filesize - %i\n", current->filesize);
        current = current->next;
    }
    return 0;
}
