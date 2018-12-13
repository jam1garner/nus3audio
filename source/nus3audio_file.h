#include <stdint.h>

typedef struct SectionHeader_t {
    char magic[4];
    uint32_t size;
} SectionHeader;

typedef struct AudioIndexSection_t {
    char magic[8]; // AUDIINDX
    uint32_t size;
    uint32_t trackCount;
} AudioIndexSection;

typedef struct TrackNumberIdSection_t {
    SectionHeader header;
    union {
        uint8_t* data;
        uint32_t* trackNumbers;
    };
} TrackNumberIdSection;

typedef struct NameOfSection_t {
    SectionHeader header;
    union {
        uint8_t* data;
        uint32_t* nameOffsets;
    };
} NameOfSection;

typedef struct AddressSection_t {
    SectionHeader header;
    union {
        uint8_t* data;
        FileEntry* entries;
    };
} AddressSection;

