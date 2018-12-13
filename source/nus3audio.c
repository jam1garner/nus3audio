#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "nus3audio.h"
#include "nus3audio_file.h"

#define NUS3_HEADER_SIZE 8
#define TNNM_HEADER_SIZE 8

void write_file(FILE* file, Nus3audioFile* nus){
    // AUDIINDX section setup
    int audiindxSectionSize = 0x10;
    uint32_t* audiindxSection = malloc(audiindxSectionSize);
    memcpy(audiindxSection, "AUDIINDX", 0x8);
    audiindxSection[2] = 0x4;
    audiindxSection[3] = nus->entryCount;

    // TNID section setup
    int tnidSectionSize = (nus->entryCount * 4) + 0x8;
    uint32_t* tnidSection = malloc(tnidSectionSize);
    memcpy(tnidSection, "TNID", 4);
    tnidSection[1] = (uint32_t)(nus->entryCount * 4);
    uint32_t* trackIds = tnidSection + 8;

    // NMOF section setup
    int nmofSectionSize = (nus->entryCount * 4) + 0x8;
    uint32_t* nmofSection = malloc(nmofSectionSize);
    memcpy(nmofSection, "NMOF", 4);
    nmofSection[1] = (uint32_t)(nus->entryCount * 4);
    uint32_t* nameOffsets = nmofSection + 8;

    // ADOF section setup
    int adofSectionSize = (nus->entryCount * 8) + 0x8;
    uint32_t* adofSection = malloc(adofSectionSize);
    memcpy(adofSection, "ADOF", 4);
    adofSection[1] = (uint32_t)(nus->entryCount * 8);
    FileEntry* addressOffsets = adofSection + 8;
    
    // First pass + String offset calculations
    int startingPosition = NUS3_HEADER_SIZE + audiindxSectionSize + tnidSectionSize + nmofSectionSize + 
                            adofSectionSize + TNNM_HEADER_SIZE;
    int currentPosition = startingPosition;
    AudioFile* currentNode = nus->head;
    for(int i = 0; currentNode; i++){
        trackIds[i] = currentNode->id;
        addressOffsets[i].fileSize = currentNode->filesize;
        nameOffsets[i] = currentPosition;
        currentPosition += strlen(currentNode->name) + 1;
        currentNode = currentNode->next;
    }
    // Size of the string section rounded to the nearest 0x4 to word allign it
    int stringSectionSize = ((currentPosition - startingPosition) + 3) & ~0x3;
    int tnnmSectionSize = TNNM_HEADER_SIZE + stringSectionSize;
    uint32_t* tnnmSection = calloc(1, tnnmSectionSize);
    memcpy(tnnmSection, "TNNM", 4);
    tnnmSection[1] = stringSectionSize;
    char* stringSection = tnnmSection + TNNM_HEADER_SIZE;
    currentNode = nus->head;
    char* stringWritePosition = stringSection;
    for(int i = 0; currentNode; i++){
        strcpy(stringWritePosition, currentNode->name);
        stringWritePosition += strlen(currentNode->name) + 1;
        currentNode = currentNode->next;
    }

    // Figure out padding (JUNK section)
    int junkSectionSize = 0;
    uint32_t* junkSection = NULL;
    int filesizeBeforePack = startingPosition + stringSectionSize;
    if (filesizeBeforePack % 0x10 != 0x8){
        // Add a JUNK section if the files won't be 0x10 alligned
        int junkSize;
        switch((filesizeBeforePack % 0x10) / 4){
            case 0:
                junkSize = 0x0;
                break;
            case 1:
                junkSize = 0xC;
                break;
            case 3:
                junkSize = 0x4;
                break;
        }
        junkSectionSize = 0x8 + junkSize;
        junkSection = calloc(1, junkSectionSize);
        memcpy(junkSection, "JUNK", 4);
        junkSection[1] = junkSize;
    }

    // Make PACK section and populate it
    int packStart = filesizeBeforePack + junkSectionSize + 0x8;
    int fileWriteOffset = packStart;
    currentNode = nus->head;
    for(int i = 0; currentNode; i++){
        addressOffsets[i].fileOffset = fileWriteOffset;
        fileWriteOffset += currentNode->filesize;
        fileWriteOffset = (fileWriteOffset + 0xF) & ~0xF;
        currentNode = currentNode->next;
    }
    int packSectionSize = (fileWriteOffset - packStart) + 0x8;
    uint32_t* packSection = calloc(1, packSectionSize);
    memcpy(packSection, "PACK", 4);
    packSection[1] = (fileWriteOffset - packStart);
    
    // Write PACK section
    uint8_t* packBase = packSection + 0x8;
    fileWriteOffset = 0;
    currentNode = nus->head;
    for(int i = 0; currentNode; i++){
        memcpy(packBase + fileWriteOffset, currentNode->data, currentNode->filesize);
        fileWriteOffset += currentNode->filesize;
        fileWriteOffset = (fileWriteOffset + 0xF) & ~0xF;
        currentNode = currentNode->next;
    }

    SectionHeader* nus3Section = malloc(sizeof(SectionHeader));
    memcpy(nus3Section->magic, "NUS3", 4);
    nus3Section->size = fileWriteOffset - NUS3_HEADER_SIZE;

    // Write generated sections to file in the order:
    // NUS3, AUDIINDX, TNID, NMOF, ADOF, TNNM, JUNK (if applicable), PACK
    fwrite(nus3Section, sizeof(SectionHeader), 1, file);
    fwrite(audiindxSection, audiindxSectionSize, 1, file);
    fwrite(tnidSection, tnidSectionSize, 1, file);
    fwrite(nmofSection, nmofSectionSize, 1, file);
    fwrite(adofSection, adofSectionSize, 1, file);
    fwrite(tnnmSection, tnnmSectionSize, 1, file);
    if(junkSection)
        fwrite(junkSection, junkSectionSize, 1, file);
    fwrite(packSection, packSectionSize, 1, file);
    free(nus3Section);
    free(audiindxSection);
    free(tnidSection);
    free(nmofSection);
    free(adofSection);
    free(tnnmSection);
    if(junkSection)
        free(junkSection);
}

Nus3audioFile* parse_file(FILE* file){
    // Read nus3bank/nus3audio header (just some magic and the filesize)
    SectionHeader* nus3Header = malloc(sizeof(SectionHeader));
    fread(nus3Header, sizeof(SectionHeader), 1, file);
    assert(!memcmp(nus3Header, "NUS3", 4));
    free(nus3Header);

    // Read basic track information (size of rest of file, )
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
        //printf("%.4s\n", temp.magic);
        if(!memcmp(&temp, "TNID", 4)){
            // Read the track ids
            trackIdSection = malloc(sizeof(TrackNumberIdSection));
            memcpy(trackIdSection, &temp, sizeof(SectionHeader));
            trackIdSection->data = malloc((size_t)temp.size);
            fread(trackIdSection->data, (size_t)temp.size, 1, file);
        }
        else if(!memcmp(&temp, "NMOF", 4)){
            // Read the name pointers
            trackNameSection = malloc(sizeof(NameOfSection));
            memcpy(trackNameSection, &temp, sizeof(SectionHeader));
            trackNameSection->data = malloc((size_t)temp.size);
            fread(trackNameSection->data, (size_t)temp.size, 1, file);
        }
        else if(!memcmp(&temp, "ADOF", 4)){
            // Read the file address/size information
            addressSection = malloc(sizeof(AddressSection));
            memcpy(addressSection, &temp, sizeof(SectionHeader));
            addressSection->data = malloc((size_t)temp.size);
            fread(addressSection->data, (size_t)temp.size, 1, file);
        }
        else if(!memcmp(&temp, "TNNM", 4)){
            // Read packed string section and store the address of it in [stringoffset]
            // to use for pointer arithmatic later
            stringOffset = ftell(file);
            stringData = malloc(temp.size);
            fread(stringData, (size_t)temp.size, 1, file);
        }
        else if(!memcmp(&temp, "JUNK", 4)){
            // Skip file padding
            fseek(file, temp.size, SEEK_CUR);
        }
        else if(!memcmp(&temp, "PACK", 4)){
            // Read packed file data, storing the address in [packOffset] for
            // the purpose of pointer arithmatic later
            packOffset = ftell(file);
            packData = malloc(temp.size);
            fread(packData, (size_t)temp.size, 1, file);
            break; 
        }
    }
    // Assert all the necessary sections exist, if they don't then rip
    assert(addressSection && trackNameSection && trackIdSection && stringData);
    Nus3audioFile* nus3audioFile = malloc(sizeof(Nus3audioFile));
    nus3audioFile->entryCount = trackCount;
    // Convert all the data into a linked list grouped by audio file
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
        // Use pointer arithmatic to point to the string in the bulk read string section
        current->name = stringData + (trackNameSection->nameOffsets[i] - stringOffset);
        current->filesize = addressSection->entries[i].fileSize;
        // Use pointer arithmatic to point to the file in the bulk read file section
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