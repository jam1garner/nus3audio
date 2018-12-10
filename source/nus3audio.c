#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "nus3audio.h"
#include "nus3audio_file.h"

Nus3audioFile* parse_file(FILE* file){
    SectionHeader* nus3Header = malloc(sizeof(SectionHeader));
    fread(nus3Header, sizeof(SectionHeader), 1, file);
    assert(!memcmp(nus3Header, "NUS3", 4));
    free(nus3Header);

    AudioIndexSection* audioInfo = malloc(sizeof(AudioIndexSection));
    fread(audioInfo, sizeof(AudioIndexSection), 1, file);
    assert(!memcmp(audioInfo, "AUDIINDX", 8));
    int trackCount = audioInfo->trackCount;
    free(audioInfo);

    AddressSection* addressSection = NULL;
    NameOfSection* trackNameSection = NULL;
    TrackNumberIdSection* trackIdSection = NULL;
    char* stringData = NULL;
    uint8_t* packData;
    int stringOffset, stringDataSize, packOffset;
    while(!feof(file)){
        SectionHeader temp;
        fread(&temp, sizeof(SectionHeader), 1, file);
        printf("%.4s\n", temp.magic);
        if(!memcmp(&temp, "TNID", 4)){
            trackIdSection = malloc(sizeof(TrackNumberIdSection));
            memcpy(trackIdSection, &temp, sizeof(SectionHeader));
            trackIdSection->data = malloc((size_t)temp.size);
            fread(trackIdSection->data, (size_t)temp.size, 1, file);
        }
        else if(!memcmp(&temp, "NMOF", 4)){
            trackNameSection = malloc(sizeof(NameOfSection));
            memcpy(trackNameSection, &temp, sizeof(SectionHeader));
            trackNameSection->data = malloc((size_t)temp.size);
            fread(trackNameSection->data, (size_t)temp.size, 1, file);
        }
        else if(!memcmp(&temp, "ADOF", 4)){
            addressSection = malloc(sizeof(AddressSection));
            memcpy(addressSection, &temp, sizeof(SectionHeader));
            addressSection->data = malloc((size_t)temp.size);
            fread(addressSection->data, (size_t)temp.size, 1, file);
        }
        else if(!memcmp(&temp, "TNNM", 4)){
            stringOffset = ftell(file);
            stringData = malloc(temp.size);
            fread(stringData, (size_t)temp.size, 1, file);
        }
        else if(!memcmp(&temp, "JUNK", 4)){
            fseek(file, temp.size, SEEK_CUR);
        }
        else if(!memcmp(&temp, "PACK", 4)){
            packOffset = ftell(file);
            packData = malloc(temp.size);
            fread(packData, (size_t)temp.size, 1, file);
            break; 
        }
    }
    assert(addressSection && trackNameSection && trackIdSection && stringData);
    Nus3audioFile* nus3audioFile = malloc(sizeof(Nus3audioFile));
    nus3audioFile->entryCount = trackCount;
    if(trackCount)
        nus3audioFile->head = malloc(sizeof(AudioFile));
    AudioFile* current;
    for(int i = 0; i < trackCount; i++){
        if(i == 0){
            current = nus3audioFile->head;
        }
        else {
            current->next = malloc(sizeof(AudioFile));
            current = current->next;
        }
        current->id = trackIdSection->trackNumbers[i];
        current->name = stringData + (trackNameSection->nameOffsets[i] - stringOffset);
        current->filesize = addressSection->entries[i].fileSize;
        current->data = packData + (addressSection->entries[i].fileOffset - packOffset);
    }

    free(addressSection);
    free(trackIdSection);
    free(trackNameSection);

    return nus3audioFile;
}

AudioFile* get_audio_file(Nus3audioFile* file, int id){
    AudioFile* current = file->head;
    while(current != NULL){
        if(current->id == id)
            return current;
    }
    return NULL;
}