#ifndef _STRING_HASH_H_
#define _STRING_HASH_H_
#include "macros.h"
#include <string.h>
#include <stdbool.h>
typedef __ssize_t hash_t;
typedef enum _ValueAssignE{
    VA_Full,VA_Written,VA_Rewritten
}ValueAssignE;
static inline hash_t StringMap_Hash(const char* str){
    hash_t hash=5381;
    int c;
    while((c=*str++)) hash=((hash<<5)+hash)+c;
    return hash;
}
static inline hash_t IntLongMap_Hash(long intl){
    return intl;//TODO
}
#define StringMap_ImplDecl(ValueType,VName)\
typedef struct StringMap_##VName##_s{\
    const size_t MaxSize;\
    size_t size;\
    char** keys;\
    ValueType* map_values;\
}StringMap_##VName##_t;\
typedef struct StringMapValue_##VName##_s{\
    bool exists;\
    ValueType value;\
}StringMapValue_##VName##_t;\
static inline hash_t StringMap_##VName##_Mod(const StringMap_##VName##_t* this,hash_t num){\
    return num%(this->MaxSize);\
}\
static inline hash_t StringMap_##VName##_SubMod(const StringMap_##VName##_t* this,hash_t negative_num_maybe){\
    return (this->MaxSize+negative_num_maybe)%(this->MaxSize);\
}\
StringMap_##VName##_t* StringMap_##VName##_new(size_t size);\
ValueAssignE StringMap_##VName##_assign(StringMap_##VName##_t* this,const char* key,ValueType map_value);\
ValueAssignE StringMap_##VName##_assign_own(StringMap_##VName##_t* this,char* key,ValueType map_value);\
bool StringMap_##VName##_erase(StringMap_##VName##_t* this,const char* key);\
bool StringMap_##VName##_erase_own(StringMap_##VName##_t* this,char* key);\
StringMapValue_##VName##_t StringMap_##VName##_read(const StringMap_##VName##_t* this,const char* key);\
StringMapValue_##VName##_t StringMap_##VName##_read_own(const StringMap_##VName##_t* this,char* key);\
StringMapValue_##VName##_t StringMap_##VName##_pop(StringMap_##VName##_t* this,const char* key);\
StringMapValue_##VName##_t StringMap_##VName##_pop_own(StringMap_##VName##_t* this,char* key);\
void StringMap_##VName##_print(const StringMap_##VName##_t* this);\
void StringMap_##VName##_print_debug(const StringMap_##VName##_t* this);\
void StringMap_##VName##_free(StringMap_##VName##_t* this);\
bool StringMap_##VName##_resize(StringMap_##VName##_t** this,size_t new_size);

#define IntLongMap_ImplDecl(ValueType,VName)\
typedef struct IntLongMap_##VName##_s{\
    const size_t MaxSize;\
    size_t size;\
    long* keys;\
    bool* key_exists;\
    ValueType* map_values;\
}IntLongMap_##VName##_t;\
typedef struct IntLongMapValue_##VName##_s{\
    bool exists;\
    ValueType value;\
}IntLongMapValue_##VName##_t;\
static inline hash_t IntLongMap_##VName##_Mod(const IntLongMap_##VName##_t* this,hash_t num){\
    return num%(this->MaxSize);\
}\
static inline hash_t IntLongMap_##VName##_SubMod(const IntLongMap_##VName##_t* this,hash_t negative_num_maybe){\
    return (this->MaxSize+negative_num_maybe)%(this->MaxSize);\
}\
IntLongMap_##VName##_t* IntLongMap_##VName##_new(size_t size);\
ValueAssignE IntLongMap_##VName##_assign(IntLongMap_##VName##_t* this,long key,ValueType map_value);\
bool IntLongMap_##VName##_erase(IntLongMap_##VName##_t* this,long key);\
IntLongMapValue_##VName##_t IntLongMap_##VName##_read(const IntLongMap_##VName##_t* this,long key);\
IntLongMapValue_##VName##_t IntLongMap_##VName##_pop(IntLongMap_##VName##_t* this,long key);\
void IntLongMap_##VName##_print(const IntLongMap_##VName##_t* this);\
void IntLongMap_##VName##_print_debug(const IntLongMap_##VName##_t* this);\
void IntLongMap_##VName##_free(IntLongMap_##VName##_t* this);\
bool IntLongMap_##VName##_resize(IntLongMap_##VName##_t** this,size_t new_size);

StringMap_ImplDecl(size_t,SizeT)
IntLongMap_ImplDecl(size_t,SizeT)
#endif