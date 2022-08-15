#ifndef _STRING_HASH_H_
#define _STRING_HASH_H_
#include "macros.h"
#include <string.h>
#include <stdbool.h>
typedef __ssize_t hash_t;
#define StringMap_ImplDecl(ValueType,TypeName)\
typedef struct StringMap_##TypeName##_s{\
    const size_t MaxSize;\
    size_t size;\
    char** string_keys;\
    ValueType* map_values;\
}StringMap_##TypeName##_t;\
static inline hash_t StringMap_##TypeName##_Hash(const char* str){\
    hash_t hash=5381;\
    int c;\
    while((c=*str++)) hash=((hash<<5)+hash)+c;\
    return hash;\
}\
static inline hash_t StringMap_##TypeName##_Mod(const StringMap_##TypeName##_t* this,hash_t num){\
    return num%(this->MaxSize);\
}\
static inline hash_t StringMap_##TypeName##_SubMod(const StringMap_##TypeName##_t* this,hash_t negative_num_maybe){\
    return (this->MaxSize+negative_num_maybe)%(this->MaxSize);\
}\
StringMap_##TypeName##_t* StringMap_##TypeName##_new(size_t size);\
bool StringMap_##TypeName##_assign(StringMap_##TypeName##_t* this,const char* key,ValueType map_value);\
bool StringMap_##TypeName##_assign_own(StringMap_##TypeName##_t* this,char* key,ValueType map_value);\
bool StringMap_##TypeName##_erase(StringMap_##TypeName##_t* this,const char* key);\
bool StringMap_##TypeName##_erase_own(StringMap_##TypeName##_t* this,char* key);\
bool StringMap_##TypeName##_read(const StringMap_##TypeName##_t* this,const char* key,ValueType* map_value);\
bool StringMap_##TypeName##_read_own(const StringMap_##TypeName##_t* this,char* key,ValueType* map_value);\
void StringMap_##TypeName##_print_debug(const StringMap_##TypeName##_t* this);\
void StringMap_##TypeName##_print(const StringMap_##TypeName##_t* this);\
void StringMap_##TypeName##_free(StringMap_##TypeName##_t* this);\
bool StringMap_##TypeName##_check_rhh_valid(const StringMap_##TypeName##_t* this);

StringMap_ImplDecl(size_t,SizeT)
#endif