#include <stdint.h>
#include <stdio.h>

typedef struct AudioFile_t {
    uint32_t id;
    char* name;
    uint32_t filesize;
    uint8_t* data;
    struct AudioFile_t* next;
} AudioFile;

typedef struct Nus3audioFile_t {
    AudioFile* head;
    int entryCount;
} Nus3audioFile;

Nus3audioFile* parse_file(FILE* file);
AudioFile* get_audio_file(Nus3audioFile* file, int id);