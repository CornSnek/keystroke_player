#ifndef _STRING_HASH_H_
#define _STRING_HASH_H_
#include "macros.h"
#include <string.h>
#include <stdbool.h>
typedef signed int long hash_t;
typedef size_t map_value_t;
typedef struct StringMap_s{
    const size_t MaxSize;
    size_t size;
    char** string_keys;
    map_value_t* map_values;
}StringMap_t;
static inline hash_t SM_Hash(const char* str){//djb2 from http://www.cse.yorku.ca/~oz/hash.html
    hash_t hash=5381;
    int c;
    while((c=*str++)) hash=((hash<<5)+hash)+c;
    return hash;
}
static inline hash_t SM_Mod(const StringMap_t* this,hash_t num){
    return num%(this->MaxSize);
}
static inline hash_t SM_SubMod(const StringMap_t* this,hash_t negative_num){//Modulus when doing any subtraction to get a positive number (Subtracting by max MaxSize only).
    return (this->MaxSize+negative_num)%(this->MaxSize);
}
StringMap_t* SM_new(size_t size);
bool SM_assign(StringMap_t* this,const char* key,map_value_t map_values);
bool SM_assign_own(StringMap_t* this,char* key,map_value_t map_value);
bool SM_erase(StringMap_t* this,const char* key);
bool SM_erase_own(StringMap_t* this,char* key);
bool SM_read(const StringMap_t* this,const char* key,map_value_t* value);
bool SM_read_own(const StringMap_t* this,char* key,map_value_t* value);
void SM_print_debug(const StringMap_t* this);
void SM_free(StringMap_t* this);
#endif