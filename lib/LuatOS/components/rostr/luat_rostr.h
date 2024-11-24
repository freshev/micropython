#ifndef LUAT_ROTSTR_H
#define LUAT_ROTSTR_H

#include "luat_base.h"
#include "lstring.h"

typedef struct luat_rostr
{
    // Standard header, 12 bytes
    char magic[4];   //Magic number, fixed to "ROST"
    uint8_t version; // Version information, starting from version 1, 1 byte
    uint8_t revert[3];// Reserve space

    //The following is the definition of version 1
    //The first is the short string information
    uint32_t short_str_count; //The total number of short strings
    uint32_t short_str_len;   //The total length of the short string
    // Then there is the long string information, currently all 0, we will consider it later.
    uint32_t long_str_count;  //The total number of long strings
    uint32_t long_str_len;    //The total length of the long string
    
    // Number of strings for each length of short string, each element is uint16_t
    uint16_t short_str_len_count[40];
    
    //The follow-up is the specific data, which needs to be calculated to get the specific area, and then traverse the strings inside
}luat_rostr_t;

typedef struct luat_rostr_short {
    TString str;
    char data[4]; // Note that the actual length is str.len + 1, and the end must be \0
    // Following is the string data, which needs to be reduced to 4 bytes
    // For example, if "abcd" is stored as 4 characters, the actual length is 16 bytes (TString header) + 4 bytes (actual data) + 1 byte (\0) = 24 bytes (21 bytes are aligned to 4 byte)
}luat_rostr_short_t;

typedef struct luat_rostr_short8 {
    TString str;
    char data[8];
}luat_rostr_short8_t;
typedef struct luat_rostr_short12 {
    TString str;
    char data[12];
}luat_rostr_short12_t;
typedef struct luat_rostr_short16 {
    TString str;
    char data[16];
}luat_rostr_short16_t;
typedef struct luat_rostr_short20 {
    TString str;
    char data[20];
}luat_rostr_short20_t;
typedef struct luat_rostr_short24 {
    TString str;
    char data[24];
}luat_rostr_short24_t;
typedef struct luat_rostr_short44 {
    TString str;
    char data[44];
}luat_rostr_short44_t;

TString* luat_rostr_get(const char *val_str, size_t len);
GCObject* luat_rostr_get_gc(const char *val_str, size_t len);

#endif
